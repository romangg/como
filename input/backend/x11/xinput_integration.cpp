/*
    SPDX-FileCopyrightText: 2016 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "xinput_integration.h"

#include "cursor.h"
#include "ge_event_mem_mover.h"
#include "input/gestures.h"
#include "input/logging.h"
#include "main.h"
#include "platform.h"
#include "screenedge.h"
#include "xinput_helpers.h"

#include "input/spies/modifier_only_shortcuts.h"
#include "platform/x11/event_filter.h"
#include <kwinglobals.h>

#include <X11/extensions/XI2proto.h>
#include <X11/extensions/XInput2.h>

#include <linux/input.h>

namespace KWin::input::backend::x11
{

static inline qreal fixed1616ToReal(FP1616 val)
{
    return (val)*1.0 / (1 << 16);
}

class XInputEventFilter : public KWin::platform::x11::event_filter
{
public:
    XInputEventFilter(int xi_opcode)
        : KWin::platform::x11::event_filter(XCB_GE_GENERIC,
                                            xi_opcode,
                                            QVector<int>{XI_RawMotion,
                                                         XI_RawButtonPress,
                                                         XI_RawButtonRelease,
                                                         XI_RawKeyPress,
                                                         XI_RawKeyRelease,
                                                         XI_TouchBegin,
                                                         XI_TouchUpdate,
                                                         XI_TouchOwnership,
                                                         XI_TouchEnd})
    {
    }
    ~XInputEventFilter() override = default;

    bool event(xcb_generic_event_t* event) override
    {
        GeEventMemMover ge(event);
        switch (ge->event_type) {
        case XI_RawKeyPress: {
            auto re = reinterpret_cast<xXIRawEvent*>(event);
            keyboard_key_pressed(re->detail - 8, re->time);
            break;
        }
        case XI_RawKeyRelease: {
            auto re = reinterpret_cast<xXIRawEvent*>(event);
            keyboard_key_released(re->detail - 8, re->time);
            break;
        }
        case XI_RawButtonPress: {
            auto e = reinterpret_cast<xXIRawEvent*>(event);
            switch (e->detail) {
            // TODO: this currently ignores left handed settings, for current usage not needed
            // if we want to use also for global mouse shortcuts, this needs to reflect state
            // correctly
            case XCB_BUTTON_INDEX_1:
                pointer_button_pressed(BTN_LEFT, e->time);
                break;
            case XCB_BUTTON_INDEX_2:
                pointer_button_pressed(BTN_MIDDLE, e->time);
                break;
            case XCB_BUTTON_INDEX_3:
                pointer_button_pressed(BTN_RIGHT, e->time);
                break;
            case XCB_BUTTON_INDEX_4:
            case XCB_BUTTON_INDEX_5:
                // vertical axis, ignore on press
                break;
                // TODO: further buttons, horizontal scrolling?
            }
        }
            if (m_x11Cursor) {
                m_x11Cursor->schedulePoll();
            }
            break;
        case XI_RawButtonRelease: {
            auto e = reinterpret_cast<xXIRawEvent*>(event);
            switch (e->detail) {
            // TODO: this currently ignores left handed settings, for current usage not needed
            // if we want to use also for global mouse shortcuts, this needs to reflect state
            // correctly
            case XCB_BUTTON_INDEX_1:
                pointer_button_released(BTN_LEFT, e->time);
                break;
            case XCB_BUTTON_INDEX_2:
                pointer_button_released(BTN_MIDDLE, e->time);
                break;
            case XCB_BUTTON_INDEX_3:
                pointer_button_released(BTN_RIGHT, e->time);
                break;
            case XCB_BUTTON_INDEX_4:
                pointer_axis_vertical(120, e->time);
                break;
            case XCB_BUTTON_INDEX_5:
                pointer_axis_vertical(-120, e->time);
                break;
                // TODO: further buttons, horizontal scrolling?
            }
        }
            if (m_x11Cursor) {
                m_x11Cursor->schedulePoll();
            }
            break;
        case XI_TouchBegin: {
            auto e = reinterpret_cast<xXIDeviceEvent*>(event);
            m_lastTouchPositions.insert(
                e->detail, QPointF(fixed1616ToReal(e->event_x), fixed1616ToReal(e->event_y)));
            break;
        }
        case XI_TouchUpdate: {
            auto e = reinterpret_cast<xXIDeviceEvent*>(event);
            const QPointF touchPosition
                = QPointF(fixed1616ToReal(e->event_x), fixed1616ToReal(e->event_y));
            if (e->detail == m_trackingTouchId) {
                const auto last = m_lastTouchPositions.value(e->detail);
                ScreenEdges::self()->gestureRecognizer()->updateSwipeGesture(
                    QSizeF(touchPosition.x() - last.x(), touchPosition.y() - last.y()));
            }
            m_lastTouchPositions.insert(e->detail, touchPosition);
            break;
        }
        case XI_TouchEnd: {
            auto e = reinterpret_cast<xXIDeviceEvent*>(event);
            if (e->detail == m_trackingTouchId) {
                ScreenEdges::self()->gestureRecognizer()->endSwipeGesture();
            }
            m_lastTouchPositions.remove(e->detail);
            m_trackingTouchId = 0;
            break;
        }
        case XI_TouchOwnership: {
            auto e = reinterpret_cast<xXITouchOwnershipEvent*>(event);
            auto it = m_lastTouchPositions.constFind(e->touchid);
            if (it == m_lastTouchPositions.constEnd()) {
                XIAllowTouchEvents(display(), e->deviceid, e->sourceid, e->touchid, XIRejectTouch);
            } else {
                if (ScreenEdges::self()->gestureRecognizer()->startSwipeGesture(it.value()) > 0) {
                    m_trackingTouchId = e->touchid;
                }
                XIAllowTouchEvents(display(),
                                   e->deviceid,
                                   e->sourceid,
                                   e->touchid,
                                   m_trackingTouchId == e->touchid ? XIAcceptTouch : XIRejectTouch);
            }
            break;
        }
        default:
            if (m_x11Cursor) {
                m_x11Cursor->schedulePoll();
            }
            break;
        }
        return false;
    }

    void setCursor(const QPointer<cursor>& cursor)
    {
        m_x11Cursor = cursor;
    }
    void setDisplay(Display* display)
    {
        m_x11Display = display;
    }

private:
    Display* display() const
    {
        return m_x11Display;
    }

    QPointer<cursor> m_x11Cursor;
    Display* m_x11Display = nullptr;
    uint32_t m_trackingTouchId = 0;
    QHash<uint32_t, QPointF> m_lastTouchPositions;
};

class XKeyPressReleaseEventFilter : public KWin::platform::x11::event_filter
{
public:
    XKeyPressReleaseEventFilter(uint32_t type)
        : KWin::platform::x11::event_filter(type)
    {
    }
    ~XKeyPressReleaseEventFilter() override = default;

    bool event(xcb_generic_event_t* event) override
    {
        xcb_key_press_event_t* ke = reinterpret_cast<xcb_key_press_event_t*>(event);
        if (ke->event == ke->root) {
            const uint8_t eventType = event->response_type & ~0x80;
            if (eventType == XCB_KEY_PRESS) {
                keyboard_key_pressed(ke->detail - 8, ke->time);
            } else {
                keyboard_key_released(ke->detail - 8, ke->time);
            }
        }
        return false;
    }
};

xinput_integration::xinput_integration(Display* display, QObject* parent)
    : QObject(parent)
    , m_x11Display(display)
{
}

xinput_integration::~xinput_integration() = default;

void xinput_integration::init()
{
    Display* dpy = display();
    int xi_opcode, event, error;
    // init XInput extension
    if (!XQueryExtension(dpy, "XInputExtension", &xi_opcode, &event, &error)) {
        qCDebug(KWIN_INPUT) << "XInputExtension not present";
        return;
    }

    // verify that the XInput extension is at at least version 2.0
    int major = 2, minor = 2;
    int result = XIQueryVersion(dpy, &major, &minor);
    if (result != Success) {
        qCDebug(KWIN_INPUT) << "Failed to init XInput 2.2, trying 2.0";
        minor = 0;
        if (XIQueryVersion(dpy, &major, &minor) != Success) {
            qCDebug(KWIN_INPUT) << "Failed to init XInput";
            return;
        }
    }
    m_hasXInput = true;
    m_xiOpcode = xi_opcode;
    m_majorVersion = major;
    m_minorVersion = minor;
    qCDebug(KWIN_INPUT) << "Has XInput support" << m_majorVersion << "." << m_minorVersion;
}

void xinput_integration::setCursor(cursor* cursor)
{
    m_x11Cursor = QPointer<x11::cursor>(cursor);
}

void xinput_integration::startListening()
{
    // this assumes KWin is the only one setting events on the root window
    // given Qt's source code this seems to be true. If it breaks, we need to change
    XIEventMask evmasks[1];
    unsigned char mask1[XIMaskLen(XI_LASTEVENT)];

    memset(mask1, 0, sizeof(mask1));

    XISetMask(mask1, XI_RawMotion);
    XISetMask(mask1, XI_RawButtonPress);
    XISetMask(mask1, XI_RawButtonRelease);
    if (m_majorVersion >= 2 && m_minorVersion >= 1) {
        // we need to listen to all events, which is only available with XInput 2.1
        XISetMask(mask1, XI_RawKeyPress);
        XISetMask(mask1, XI_RawKeyRelease);
    }
    if (m_majorVersion >= 2 && m_minorVersion >= 2) {
        // touch events since 2.2
        XISetMask(mask1, XI_TouchBegin);
        XISetMask(mask1, XI_TouchUpdate);
        XISetMask(mask1, XI_TouchOwnership);
        XISetMask(mask1, XI_TouchEnd);
    }

    evmasks[0].deviceid = XIAllMasterDevices;
    evmasks[0].mask_len = sizeof(mask1);
    evmasks[0].mask = mask1;
    XISelectEvents(display(), rootWindow(), evmasks, 1);

    m_xiEventFilter.reset(new XInputEventFilter(m_xiOpcode));
    m_xiEventFilter->setCursor(m_x11Cursor);
    m_xiEventFilter->setDisplay(display());
    m_keyPressFilter.reset(new XKeyPressReleaseEventFilter(XCB_KEY_PRESS));
    m_keyReleaseFilter.reset(new XKeyPressReleaseEventFilter(XCB_KEY_RELEASE));

    // install the input event spies also relevant for X11 platform
    kwinApp()->input_redirect->installInputEventSpy(new input::modifier_only_shortcuts_spy);
}

}
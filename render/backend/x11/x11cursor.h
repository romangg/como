/*
    SPDX-FileCopyrightText: 2013 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef KWIN_X11CURSOR_H
#define KWIN_X11CURSOR_H

#include "input/cursor.h"

#include <memory>

namespace KWin::render::backend::x11
{
class XFixesCursorEventFilter;

class KWIN_EXPORT X11Cursor : public input::cursor
{
    Q_OBJECT
public:
    X11Cursor(QObject* parent, bool xInputSupport = false);
    ~X11Cursor() override;

    void schedulePoll()
    {
        m_needsPoll = true;
    }

    /**
     * @internal
     *
     * Called from X11 event handler.
     */
    void notifyCursorChanged();

protected:
    xcb_cursor_t getX11Cursor(input::cursor_shape shape) override;
    xcb_cursor_t getX11Cursor(const QByteArray& name) override;
    void doSetPos() override;
    void doGetPos() override;
    void doStartMousePolling() override;
    void doStopMousePolling() override;
    void doStartCursorTracking() override;
    void doStopCursorTracking() override;

private Q_SLOTS:
    /**
     * Because of QTimer's and the impossibility to get events for all mouse
     * movements (at least I haven't figured out how) the position needs
     * to be also refetched after each return to the event loop.
     */
    void resetTimeStamp();
    void mousePolled();
    void aboutToBlock();

private:
    xcb_cursor_t createCursor(const QByteArray& name);
    QHash<QByteArray, xcb_cursor_t> m_cursors;
    xcb_timestamp_t m_timeStamp;
    uint16_t m_buttonMask;
    QTimer* m_resetTimeStampTimer;
    QTimer* m_mousePollingTimer;
    bool m_hasXInput;
    bool m_needsPoll;

    std::unique_ptr<XFixesCursorEventFilter> m_xfixesFilter;

    friend class Cursor;
};

}

#endif

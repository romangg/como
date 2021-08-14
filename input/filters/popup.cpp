/*
    SPDX-FileCopyrightText: 2017  Martin Graesslin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "popup.h"
#include "wayland_server.h"

#include "win/deco.h"
#include "win/geo.h"
#include "win/transient.h"
#include "win/util.h"
#include "win/wayland/window.h"

#include <QMouseEvent>
#include <Wrapland/Server/keyboard.h>
#include <Wrapland/Server/seat.h>

namespace KWin::input
{

popup_filter::popup_filter()
    : QObject()
{
    connect(
        waylandServer(), &WaylandServer::window_added, this, &popup_filter::handle_window_added);
}

void popup_filter::handle_window_added(win::wayland::window* window)
{
    if (contains(m_popups, window)) {
        return;
    }
    if (window->transient()->input_grab) {
        // TODO: verify that the Toplevel is allowed as a popup
        connect(
            window,
            &Toplevel::windowShown,
            this,
            [this, window] { handle_window_added(window); },
            Qt::UniqueConnection);
        connect(window,
                &Toplevel::windowClosed,
                this,
                &popup_filter::handle_window_removed,
                Qt::UniqueConnection);
        m_popups.push_back(window);
    }
}

void popup_filter::handle_window_removed(Toplevel* window)
{
    remove_all(m_popups, window);
}
bool popup_filter::pointerEvent(QMouseEvent* event, quint32 nativeButton)
{
    Q_UNUSED(nativeButton)
    if (m_popups.empty()) {
        return false;
    }
    if (event->type() == QMouseEvent::MouseButtonPress) {
        auto focus_window = kwinApp()->input_redirect->findToplevel(event->globalPos());
        if (!focus_window || !win::belong_to_same_client(focus_window, m_popups.back())) {
            // a press on a window (or no window) not belonging to the popup window
            cancelPopups();
            // filter out this press
            return true;
        }
        if (focus_window && win::decoration(focus_window)) {
            // Test whether it is on the decoration.
            auto const content_rect
                = focus_window->frameGeometry() - win::frame_margins(focus_window);
            if (!content_rect.contains(event->globalPos())) {
                cancelPopups();
                return true;
            }
        }
    }
    return false;
}

bool popup_filter::keyEvent(QKeyEvent* event)
{
    if (m_popups.empty()) {
        return false;
    }

    auto seat = waylandServer()->seat();

    auto last = m_popups.back();
    if (last->surface() == nullptr) {
        return false;
    }

    seat->setFocusedKeyboardSurface(last->surface());
    switch (event->type()) {
    case QEvent::KeyPress:
        seat->keyPressed(event->nativeScanCode());
        break;
    case QEvent::KeyRelease:
        seat->keyReleased(event->nativeScanCode());
        break;
    default:
        break;
    }

    return true;
}

void popup_filter::cancelPopups()
{
    while (!m_popups.empty()) {
        m_popups.back()->cancel_popup();
        m_popups.pop_back();
    }
}

}
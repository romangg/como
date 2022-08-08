/*
    SPDX-FileCopyrightText: 2022 Francesco Sorrentino <francesco.sorr@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include <QObject>
#include <Wrapland/Server/surface.h>

namespace KWin::win::wayland
{

// Check if @p window inhibits idle.
template<typename Device, typename Win>
void idle_update(Device& idle, Win& window)
{
    auto const is_visible = window.isShown() && window.isOnCurrentDesktop();

    if (is_visible && window.surface && window.surface->inhibitsIdle()) {
        if (!window.inhibit_idle) {
            window.inhibit_idle = true;
            idle.inhibit();
        }
    } else {
        if (window.inhibit_idle) {
            window.inhibit_idle = false;
            idle.uninhibit();
        }
    }
}

// Setup @p window's connections to @p idle inhibition;
// use only for windows with control.
template<typename Device, typename Win>
void idle_setup(Device& idle, Win& window)
{
    auto update = [&idle, &window] { idle_update(idle, window); };
    auto qwin = window.qobject.get();

    QObject::connect(window.surface, &Wrapland::Server::Surface::inhibitsIdleChanged, qwin, update);
    QObject::connect(qwin, &Win::qobject_t::desktopChanged, qwin, update);
    QObject::connect(qwin, &Win::qobject_t::clientMinimized, qwin, update);
    QObject::connect(qwin, &Win::qobject_t::clientUnminimized, qwin, update);
    QObject::connect(qwin, &Win::qobject_t::windowHidden, qwin, update);
    QObject::connect(qwin, &Win::qobject_t::windowShown, qwin, update);
    QObject::connect(qwin, &Win::qobject_t::closed, qwin, [&idle, &window](auto) {
        if (window.inhibit_idle) {
            window.inhibit_idle = false;
            idle.uninhibit();
        }
    });

    idle_update(idle, window);
}

}

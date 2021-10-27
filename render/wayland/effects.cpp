/*
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "effects.h"

#include "toplevel.h"
#include "wayland_server.h"
#include "win/wayland/window.h"

namespace KWin::render::wayland
{

effects_handler_impl::effects_handler_impl(render::compositor* compositor, Scene* scene)
    : EffectsHandlerImpl(compositor, scene)
{
    QObject::connect(waylandServer(), &WaylandServer::window_added, this, [this](auto c) {
        if (c->readyForPainting()) {
            slotXdgShellClientShown(c);
        } else {
            QObject::connect(
                c, &Toplevel::windowShown, this, &effects_handler_impl::slotXdgShellClientShown);
        }
    });
    auto const clients = waylandServer()->windows;
    for (auto c : clients) {
        if (c->readyForPainting()) {
            setupAbstractClientConnections(c);
        } else {
            QObject::connect(
                c, &Toplevel::windowShown, this, &effects_handler_impl::slotXdgShellClientShown);
        }
    }
}

EffectWindow* effects_handler_impl::findWindow(Wrapland::Server::Surface* surf) const
{
    if (auto win = waylandServer()->find_window(surf)) {
        return win->effectWindow();
    }
    return nullptr;
}

Wrapland::Server::Display* effects_handler_impl::waylandDisplay() const
{
    return waylandServer()->display();
}

}
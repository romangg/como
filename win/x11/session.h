/*
    SPDX-FileCopyrightText: 2022 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "base/x11/xcb/property.h"
#include "utils/algorithm.h"
#include "utils/blocker.h"

#include <QByteArray>

namespace KWin::win::x11
{

template<typename Space, typename Window>
void restore_session_stacking_order(Space* space, Window* c)
{
    if (c->sm_stacking_order < 0) {
        return;
    }

    blocker block(space->stacking.order);
    remove_all(space->stacking.order.pre_stack, c);

    for (auto it = space->stacking.order.pre_stack.begin(); // from bottom
         it != space->stacking.order.pre_stack.end();
         ++it) {
        auto current = dynamic_cast<Window*>(*it);
        if (!current) {
            continue;
        }
        if (current->sm_stacking_order > c->sm_stacking_order) {
            space->stacking.order.pre_stack.insert(it, c);
            return;
        }
    }
    space->stacking.order.pre_stack.push_back(c);
}

template<typename Win>
QByteArray get_session_id(Win const& win)
{
    QByteArray result
        = base::x11::xcb::string_property(win.xcb_window, win.space.atoms->sm_client_id);
    if (result.isEmpty() && win.m_wmClientLeader && win.m_wmClientLeader != win.xcb_window) {
        result
            = base::x11::xcb::string_property(win.m_wmClientLeader, win.space.atoms->sm_client_id);
    }
    return result;
}

/// Returns command property for this window, taken either from its window or from the leader.
template<typename Win>
QByteArray get_wm_command(Win const& win)
{
    QByteArray result = base::x11::xcb::string_property(win.xcb_window, XCB_ATOM_WM_COMMAND);
    if (result.isEmpty() && win.m_wmClientLeader && win.m_wmClientLeader != win.xcb_window) {
        result = base::x11::xcb::string_property(win.m_wmClientLeader, XCB_ATOM_WM_COMMAND);
    }
    result.replace(0, ' ');
    return result;
}

}

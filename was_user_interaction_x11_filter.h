/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2017 Martin Flöser <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#ifndef KWIN_WAS_USER_INTERACTION_X11_FILTER_H
#define KWIN_WAS_USER_INTERACTION_X11_FILTER_H

#include "base/x11/event_filter.h"

namespace KWin
{

class KWIN_EXPORT WasUserInteractionX11Filter : public base::x11::event_filter
{
public:
    explicit WasUserInteractionX11Filter();

    bool event(xcb_generic_event_t *event) override;
};

}

#endif

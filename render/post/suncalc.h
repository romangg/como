/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright 2017 Roman Gilg <subdiff@gmail.com>

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
#pragma once

#include "kwin_export.h"

#include <QDate>
#include <QPair>
#include <QTime>

namespace KWin::render::post
{

/**
 * Calculates for a given location and date two of the
 * following sun timings in their temporal order:
 * - Nautical dawn and sunrise for the morning
 * - Sunset and nautical dusk for the evening
 * @since 5.12
 */

KWIN_EXPORT QPair<QDateTime, QDateTime> calculate_sun_timings(const QDateTime& dateTime,
                                                              double latitude,
                                                              double longitude,
                                                              bool at_morning);

}

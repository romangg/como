/*
    SPDX-FileCopyrightText: 2022 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <climits>
#include <cstddef>
#include <vector>

namespace KWin::base
{

template<typename Output>
size_t get_nearest_output(std::vector<Output*> const& outputs, QPoint const& pos)
{
    size_t best_output{0};
    size_t index{0};
    auto min_distance = INT_MAX;

    for (auto const& output : outputs) {
        auto const& geo = output->geometry();
        if (geo.contains(pos)) {
            return index;
        }

        auto distance = QPoint(geo.topLeft() - pos).manhattanLength();
        distance = qMin(distance, QPoint(geo.topRight() - pos).manhattanLength());
        distance = qMin(distance, QPoint(geo.bottomRight() - pos).manhattanLength());
        distance = qMin(distance, QPoint(geo.bottomLeft() - pos).manhattanLength());

        if (distance < min_distance) {
            min_distance = distance;
            best_output = index;
        }
        index++;
    }

    return best_output;
}

template<typename Base>
void set_current_output(Base& base, int output)
{
    if (base.topology.current == output) {
        return;
    }
    auto old_topo = base.topology;
    base.topology.current = output;
    Q_EMIT base.topology_changed(old_topo, base.topology);
}

template<typename Base>
void set_current_output_by_position(Base& base, QPoint const& pos)
{
    set_current_output(base, get_nearest_output(base.get_outputs(), pos));
}

}

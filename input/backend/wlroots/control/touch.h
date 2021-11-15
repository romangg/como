/*
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "input/control/touch.h"

#include <QMatrix4x4>
#include <QSizeF>
#include <libinput.h>

namespace KWin::input::backend::wlroots
{

class touch_control : public input::control::touch
{
    Q_OBJECT

public:
    touch_control(libinput_device* dev, input::platform* platform);
    ~touch_control() override = default;

    bool supports_disable_events() const override;
    bool is_enabled() const override;
    bool set_enabled_impl(bool enabled) override;

    QSizeF size() const override;

    bool supports_gesture() const override;

    bool supports_calibration_matrix() const override;
    QMatrix4x4 default_calibration_matrix() const override;

    bool set_orientation_impl(QMatrix4x4 const& matrix) override;

    libinput_device* dev;
};

}

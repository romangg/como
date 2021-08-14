/*
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <KLocalizedString>
#include <QString>

namespace KWin::input
{

inline QString translated_keyboard_layout(QString const& layout)
{
    return i18nd("xkeyboard-config", layout.toUtf8().constData());
}

}
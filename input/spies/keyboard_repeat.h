/*
    SPDX-FileCopyrightText: 2016 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2017 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "input/event_spy.h"

#include <QObject>

class QTimer;

namespace KWin::input
{
class xkb;

class keyboard_repeat_spy : public QObject, public input::event_spy
{
    Q_OBJECT
public:
    explicit keyboard_repeat_spy(xkb* xkb);
    ~keyboard_repeat_spy() override;

    void keyEvent(input::KeyEvent* event) override;

Q_SIGNALS:
    void keyRepeat(quint32 key, quint32 time);

private:
    void handleKeyRepeat();
    QTimer* m_timer;
    xkb* m_xkb;
    quint32 m_time;
    quint32 m_key = 0;
};

}
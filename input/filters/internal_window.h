/*
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "../event_filter.h"

namespace KWin::input
{

class internal_window_filter : public event_filter
{
public:
    bool keyEvent(QKeyEvent* event) override;
    bool touchDown(qint32 id, const QPointF& pos, quint32 time) override;
    bool touchMotion(qint32 id, const QPointF& pos, quint32 time) override;
    bool touchUp(qint32 id, quint32 time) override;
    bool pointerEvent(QMouseEvent* event, quint32 nativeButton) override;
    bool wheelEvent(QWheelEvent* event) override;

private:
    QSet<qint32> m_pressedIds;
    QPointF m_lastGlobalTouchPos;
    QPointF m_lastLocalTouchPos;
};

}
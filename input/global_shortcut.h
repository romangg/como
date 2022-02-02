/*
    SPDX-FileCopyrightText: 2013 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "kwinglobals.h"

#include <QKeySequence>
#include <QSharedPointer>

class QAction;

namespace KWin::input
{
class swipe_gesture;
class pinch_gesture;

struct KeyboardShortcut {
    QKeySequence sequence;
    bool operator==(const KeyboardShortcut& rhs) const
    {
        return sequence == rhs.sequence;
    }
};

struct PointerButtonShortcut {
    Qt::KeyboardModifiers pointerModifiers;
    Qt::MouseButtons pointerButtons;
    bool operator==(const PointerButtonShortcut& rhs) const
    {
        return pointerModifiers == rhs.pointerModifiers && pointerButtons == rhs.pointerButtons;
    }
};

struct PointerAxisShortcut {
    Qt::KeyboardModifiers axisModifiers;
    PointerAxisDirection axisDirection;
    bool operator==(const PointerAxisShortcut& rhs) const
    {
        return axisModifiers == rhs.axisModifiers && axisDirection == rhs.axisDirection;
    }
};

struct SwipeShortcut {
    SwipeDirection direction;
    uint fingerCount;
    bool operator==(const SwipeShortcut& rhs) const
    {
        return direction == rhs.direction && fingerCount == rhs.fingerCount;
    }
};
struct RealtimeFeedbackSwipeShortcut {
    SwipeDirection direction;
    std::function<void(qreal)> progressCallback;
    uint fingerCount;

    template<typename T>
    bool operator==(const T& rhs) const
    {
        return direction == rhs.direction && fingerCount == rhs.fingerCount;
    }
};
struct PinchShortcut {
    PinchDirection direction;
    uint fingerCount;
    bool operator==(const PinchShortcut& rhs) const
    {
        return direction == rhs.direction && fingerCount == rhs.fingerCount;
    }
};

struct RealtimeFeedbackPinchShortcut {
    PinchDirection direction;
    std::function<void(qreal)> scaleCallback;
    uint fingerCount;

    template<typename T>
    bool operator==(const T& rhs) const
    {
        return direction == rhs.direction && fingerCount == rhs.fingerCount;
    }
};

using Shortcut = std::variant<KeyboardShortcut,
                              PointerButtonShortcut,
                              PointerAxisShortcut,
                              SwipeShortcut,
                              RealtimeFeedbackSwipeShortcut,
                              PinchShortcut,
                              RealtimeFeedbackPinchShortcut>;

class global_shortcut
{
public:
    global_shortcut(Shortcut&& shortcut, QAction* action);
    ~global_shortcut();

    void invoke() const;
    QAction* action() const;
    Shortcut const& shortcut() const;
    swipe_gesture* swipeGesture() const;
    pinch_gesture* pinchGesture() const;

private:
    QSharedPointer<swipe_gesture> m_swipeGesture;
    QSharedPointer<pinch_gesture> m_pinchGesture;
    Shortcut m_shortcut{};
    QAction* m_action{nullptr};
};

}

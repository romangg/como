/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2016, 2017 Martin Gräßlin <mgraesslin@kde.org>

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
#include "modifier_only_shortcuts.h"
#include "input/event.h"
#include "options.h"
#include "screenlockerwatcher.h"
#include "workspace.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>

namespace KWin::input
{

modifier_only_shortcuts_spy::modifier_only_shortcuts_spy()
    : QObject()
    , event_spy()
{
    connect(ScreenLockerWatcher::self(),
            &ScreenLockerWatcher::locked,
            this,
            &modifier_only_shortcuts_spy::reset);
}

modifier_only_shortcuts_spy::~modifier_only_shortcuts_spy() = default;

void modifier_only_shortcuts_spy::keyEvent(KeyEvent* event)
{
    if (event->isAutoRepeat()) {
        return;
    }
    if (event->type() == QEvent::KeyPress) {
        const bool wasEmpty = m_pressedKeys.isEmpty();
        m_pressedKeys.insert(event->nativeScanCode());
        if (wasEmpty && m_pressedKeys.size() == 1 && !ScreenLockerWatcher::self()->isLocked()
            && m_buttonPressCount == 0 && m_cachedMods == Qt::NoModifier) {
            m_modifier = Qt::KeyboardModifier(int(event->modifiersRelevantForGlobalShortcuts()));
        } else {
            m_modifier = Qt::NoModifier;
        }
    } else if (!m_pressedKeys.isEmpty()) {
        m_pressedKeys.remove(event->nativeScanCode());
        if (m_pressedKeys.isEmpty()
            && event->modifiersRelevantForGlobalShortcuts() == Qt::NoModifier
            && !workspace()->globalShortcutsDisabled()) {
            if (m_modifier != Qt::NoModifier) {
                const auto list = options->modifierOnlyDBusShortcut(m_modifier);
                if (list.size() >= 4) {
                    auto call = QDBusMessage::createMethodCall(
                        list.at(0), list.at(1), list.at(2), list.at(3));
                    QVariantList args;
                    for (int i = 4; i < list.size(); ++i) {
                        args << list.at(i);
                    }
                    call.setArguments(args);
                    QDBusConnection::sessionBus().asyncCall(call);
                }
            }
        }
        m_modifier = Qt::NoModifier;
    } else {
        m_modifier = Qt::NoModifier;
    }
    m_cachedMods = event->modifiersRelevantForGlobalShortcuts();
}

void modifier_only_shortcuts_spy::pointerEvent(MouseEvent* event)
{
    if (event->type() == QEvent::MouseMove) {
        return;
    }
    if (event->type() == QEvent::MouseButtonPress) {
        m_buttonPressCount++;
    } else if (event->type() == QEvent::MouseButtonRelease) {
        m_buttonPressCount--;
    }
    reset();
}

void modifier_only_shortcuts_spy::wheelEvent(WheelEvent* event)
{
    Q_UNUSED(event)
    reset();
}

}
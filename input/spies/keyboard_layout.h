/*
    SPDX-FileCopyrightText: 2016 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2017 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "../event_spy.h"

#include <QObject>
#include <QVector>

#include <KConfigGroup>
#include <KSharedConfig>

typedef uint32_t xkb_layout_index_t;

class QAction;
class QDBusArgument;

namespace KWin::input
{
class xkb;

namespace dbus
{
class keyboard_layout;
}

namespace KeyboardLayoutSwitching
{
class Policy;
}

class keyboard_layout_spy : public QObject, public input::event_spy
{
    Q_OBJECT
public:
    explicit keyboard_layout_spy(xkb* xkb, const KSharedConfigPtr& config);

    ~keyboard_layout_spy() override;

    void init();

    void checkLayoutChange(uint previousLayout);
    void switchToNextLayout();
    void switchToPreviousLayout();
    void resetLayout();

Q_SIGNALS:
    void layoutChanged(uint index);
    void layoutsReconfigured();

private Q_SLOTS:
    void reconfigure();

private:
    void initDBusInterface();
    void notifyLayoutChange();
    void switchToLayout(xkb_layout_index_t index);
    void loadShortcuts();
    xkb* m_xkb;
    xkb_layout_index_t m_layout = 0;
    KConfigGroup m_configGroup;
    QVector<QAction*> m_layoutShortcuts;
    dbus::keyboard_layout* m_dbusInterface = nullptr;
    KeyboardLayoutSwitching::Policy* m_policy = nullptr;
};

}
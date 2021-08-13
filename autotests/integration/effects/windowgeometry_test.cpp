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
#include "effect_builtins.h"
#include "effectloader.h"
#include "effects.h"
#include "kwin_wayland_test.h"
#include "platform.h"
#include "render/compositor.h"
#include "wayland_server.h"
#include "workspace.h"

#include "win/wayland/window.h"

#include <KConfigGroup>

#include <Wrapland/Client/buffer.h>
#include <Wrapland/Client/surface.h>

using namespace Wrapland::Client;

namespace KWin
{

class WindowGeometryTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testStartup();
};

void WindowGeometryTest::initTestCase()
{
    qRegisterMetaType<win::wayland::window*>();
    qRegisterMetaType<KWin::Effect*>();

    QSignalSpy workspaceCreatedSpy(kwinApp(), &Application::workspaceCreated);
    QVERIFY(workspaceCreatedSpy.isValid());
    kwinApp()->platform->setInitialWindowSize(QSize(1280, 1024));

    // disable all effects - we don't want to have it interact with the rendering
    auto config = KSharedConfig::openConfig(QString(), KConfig::SimpleConfig);
    KConfigGroup plugins(config, QStringLiteral("Plugins"));
    ScriptedEffectLoader loader;
    const auto builtinNames = BuiltInEffects::availableEffectNames() << loader.listOfKnownEffects();
    for (QString name : builtinNames) {
        plugins.writeEntry(name + QStringLiteral("Enabled"), false);
    }
    plugins.writeEntry(BuiltInEffects::nameForEffect(BuiltInEffect::WindowGeometry)
                           + QStringLiteral("Enabled"),
                       true);

    config->sync();
    kwinApp()->setConfig(config);

    qputenv("KWIN_EFFECTS_FORCE_ANIMATIONS", "1");
    kwinApp()->start();
    QVERIFY(workspaceCreatedSpy.size() || workspaceCreatedSpy.wait());
    QVERIFY(render::compositor::self());
}

void WindowGeometryTest::init()
{
    Test::setup_wayland_connection();
}

void WindowGeometryTest::cleanup()
{
    Test::destroy_wayland_connection();
}

void WindowGeometryTest::testStartup()
{
    // just a test to load the effect to verify it doesn't crash
    EffectsHandlerImpl* e = static_cast<EffectsHandlerImpl*>(effects);
    QVERIFY(e->isEffectLoaded(BuiltInEffects::nameForEffect(BuiltInEffect::WindowGeometry)));
}

}

WAYLANDTEST_MAIN(KWin::WindowGeometryTest)
#include "windowgeometry_test.moc"

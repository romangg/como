/*
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "kwin_wayland_test.h"

#include "platform.h"
#include "wayland_server.h"
#include "workspace.h"

#include "win/control.h"
#include "win/move.h"
#include "win/wayland/window.h"
#include "win/wayland/xdg_activation.h"

#include <Wrapland/Client/surface.h>
#include <Wrapland/Client/xdg_activation_v1.h>
#include <Wrapland/Server/surface.h>
#include <Wrapland/Server/xdg_activation_v1.h>

namespace Clt = Wrapland::Client;

namespace KWin
{

constexpr char socket_name[] = "wayland-xdg-activation-test-0";

class xdg_activation_test : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void test_single_client();
    void test_multi_client();
};

void xdg_activation_test::initTestCase()
{
    qRegisterMetaType<win::wayland::window*>();
    qRegisterMetaType<std::string>();

    QSignalSpy workspace_spy(kwinApp(), &Application::workspaceCreated);
    QVERIFY(workspace_spy.isValid());
    kwinApp()->platform()->setInitialWindowSize(QSize(1280, 1024));
    kwinApp()->start();
    QVERIFY(workspace_spy.size() || workspace_spy.wait());
}

void xdg_activation_test::init()
{
    Test::setup_wayland_connection(Test::AdditionalWaylandInterface::Seat
                                   | Test::AdditionalWaylandInterface::XdgActivation);
}

void xdg_activation_test::cleanup()
{
    Test::destroy_wayland_connection();
}

void xdg_activation_test::test_single_client()
{
    // Check that XDG Activation works for two different surfaces of a single client.

    std::unique_ptr<Clt::Surface> surface1(Test::create_surface());
    auto shell_surface1 = Test::create_xdg_shell_toplevel(surface1);
    auto window1 = Test::render_and_wait_for_shown(surface1, QSize(200, 100), Qt::red);
    QVERIFY(window1);
    QCOMPARE(win::render_geometry(window1).size(), QSize(200, 100));
    QCOMPARE(window1->frameGeometry().size(), QSize(200, 100));
    QVERIFY(window1->control->active());
    QCOMPARE(workspace()->activeClient(), window1);

    std::unique_ptr<Clt::Surface> surface2(Test::create_surface());
    auto shell_surface2 = Test::create_xdg_shell_toplevel(surface2);
    auto window2 = Test::render_and_wait_for_shown(surface2, QSize(400, 200), Qt::blue);
    QVERIFY(window2);
    QCOMPARE(win::render_geometry(window2).size(), QSize(400, 200));
    QCOMPARE(window2->frameGeometry().size(), QSize(400, 200));
    QVERIFY(window2->control->active());
    QCOMPARE(workspace()->activeClient(), window2);

    auto activation = Test::get_client().interfaces.xdg_activation.get();
    QVERIFY(activation);

    auto server_activation = waylandServer()->xdg_activation;
    QVERIFY(server_activation);
    QSignalSpy token_spy(server_activation, &Wrapland::Server::XdgActivationV1::token_requested);
    QVERIFY(token_spy.isValid());

    auto token = std::unique_ptr<Clt::XdgActivationTokenV1>(activation->create_token());
    token->set_serial(0, Test::get_client().interfaces.seat.get());
    token->set_surface(surface2.get());
    token->set_app_id("testclient1");
    token->commit();

    QVERIFY(token_spy.wait());
    auto server_token = token_spy.front().front().value<Wrapland::Server::XdgActivationTokenV1*>();
    QCOMPARE(server_token->app_id(), "testclient1");

    auto const token_string = workspace()->activation->token;

    QSignalSpy done_spy(token.get(), &Clt::XdgActivationTokenV1::done);
    QVERIFY(done_spy.isValid());
    QVERIFY(done_spy.wait());

    QCOMPARE(done_spy.front().front().value<std::string>(), token_string);

    activation->activate(token_string, surface1.get());

    QSignalSpy xdg_activate_spy(server_activation, &Wrapland::Server::XdgActivationV1::activate);
    QVERIFY(xdg_activate_spy.isValid());
    QSignalSpy activated_spy(workspace(), &Workspace::clientActivated);
    QVERIFY(activated_spy.isValid());

    QVERIFY(activated_spy.wait());
    QVERIFY(!xdg_activate_spy.empty());

    QCOMPARE(xdg_activate_spy.front().front().value<std::string>(), token_string);
    QCOMPARE(xdg_activate_spy.front().back().value<Wrapland::Server::Surface*>(),
             window1->surface());
    QVERIFY(window1->control->active());
    QCOMPARE(workspace()->activeClient(), window1);
}

void xdg_activation_test::test_multi_client()
{
    // Check that XDG Activation works for two different clients.

    std::unique_ptr<Clt::Surface> surface1(Test::create_surface());
    auto shell_surface1 = Test::create_xdg_shell_toplevel(surface1);
    auto window1 = Test::render_and_wait_for_shown(surface1, QSize(200, 100), Qt::red);
    QVERIFY(window1);
    QCOMPARE(win::render_geometry(window1).size(), QSize(200, 100));
    QCOMPARE(window1->frameGeometry().size(), QSize(200, 100));
    QVERIFY(window1->control->active());
    QCOMPARE(workspace()->activeClient(), window1);

    // Create a second client.
    Test::setup_wayland_connection(Test::AdditionalWaylandInterface::Seat
                                   | Test::AdditionalWaylandInterface::XdgActivation);
    auto& client2 = Test::get_all_clients().back();

    std::unique_ptr<Clt::Surface> surface2(Test::create_surface(client2));
    auto shell_surface2 = Test::create_xdg_shell_toplevel(client2, surface2);
    auto window2 = Test::render_and_wait_for_shown(client2, surface2, QSize(400, 200), Qt::blue);
    QVERIFY(window2);
    QCOMPARE(win::render_geometry(window2).size(), QSize(400, 200));
    QCOMPARE(window2->frameGeometry().size(), QSize(400, 200));
    QVERIFY(window2->control->active());
    QCOMPARE(workspace()->activeClient(), window2);

    auto activation2 = client2.interfaces.xdg_activation.get();
    QVERIFY(activation2);

    auto server_activation = waylandServer()->xdg_activation;
    QVERIFY(server_activation);
    QSignalSpy token_spy(server_activation, &Wrapland::Server::XdgActivationV1::token_requested);
    QVERIFY(token_spy.isValid());

    auto token = std::unique_ptr<Clt::XdgActivationTokenV1>(activation2->create_token());
    token->set_serial(0, client2.interfaces.seat.get());
    token->set_surface(surface2.get());
    token->set_app_id("testclient1");
    token->commit();

    QVERIFY(token_spy.wait());
    auto server_token = token_spy.front().front().value<Wrapland::Server::XdgActivationTokenV1*>();
    QCOMPARE(server_token->app_id(), "testclient1");

    auto const token_string = workspace()->activation->token;

    QSignalSpy done_spy(token.get(), &Clt::XdgActivationTokenV1::done);
    QVERIFY(done_spy.isValid());
    QVERIFY(done_spy.wait());

    QCOMPARE(done_spy.front().front().value<std::string>(), token_string);

    auto activation1 = Test::get_client().interfaces.xdg_activation.get();
    activation1->activate(token_string, surface1.get());

    QSignalSpy xdg_activate_spy(server_activation, &Wrapland::Server::XdgActivationV1::activate);
    QVERIFY(xdg_activate_spy.isValid());
    QSignalSpy activated_spy(workspace(), &Workspace::clientActivated);
    QVERIFY(activated_spy.isValid());

    QVERIFY(activated_spy.wait());
    QVERIFY(!xdg_activate_spy.empty());

    QCOMPARE(xdg_activate_spy.front().front().value<std::string>(), token_string);
    QCOMPARE(xdg_activate_spy.front().back().value<Wrapland::Server::Surface*>(),
             window1->surface());
    QVERIFY(window1->control->active());
    QCOMPARE(workspace()->activeClient(), window1);
}

}

WAYLANDTEST_MAIN(KWin::xdg_activation_test)
#include "xdg_activation.moc"
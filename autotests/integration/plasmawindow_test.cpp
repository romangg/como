/********************************************************************
KWin - the KDE window manager
This file is part of the KDE project.

Copyright (C) 2016 Martin Gräßlin <mgraesslin@kde.org>

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
#include "kwin_wayland_test.h"
#include "platform.h"
#include "input/cursor.h"
#include "screenedge.h"
#include "screens.h"
#include "wayland_server.h"
#include "workspace.h"
#include <kwineffects.h>

#include "win/geo.h"
#include "win/wayland/window.h"
#include "win/x11/window.h"

#include <Wrapland/Client/compositor.h>
#include <Wrapland/Client/plasmawindowmanagement.h>
#include <Wrapland/Client/surface.h>
#include <Wrapland/Server/seat.h>
//screenlocker
#include <KScreenLocker/KsldApp>

#include <QPainter>
#include <QRasterWindow>

#include <netwm.h>
#include <xcb/xcb_icccm.h>

using namespace Wrapland::Client;

namespace KWin
{

static const QString s_socketName = QStringLiteral("wayland_test_kwin_plasma-window-0");

class PlasmaWindowTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void testCreateDestroyX11PlasmaWindow();
    void testInternalWindowNoPlasmaWindow();
    void testPopupWindowNoPlasmaWindow();
    void testLockScreenNoPlasmaWindow();
    void testDestroyedButNotUnmapped();

private:
    PlasmaWindowManagement *m_windowManagement = nullptr;
    Wrapland::Client::Compositor *m_compositor = nullptr;
};

void PlasmaWindowTest::initTestCase()
{
    qRegisterMetaType<win::wayland::window*>();
    qRegisterMetaType<KWin::win::x11::window*>();

    QSignalSpy workspaceCreatedSpy(kwinApp(), &Application::workspaceCreated);
    QVERIFY(workspaceCreatedSpy.isValid());
    kwinApp()->platform()->setInitialWindowSize(QSize(1280, 1024));
    QVERIFY(waylandServer()->init(s_socketName.toLocal8Bit()));

    kwinApp()->start();
    QMetaObject::invokeMethod(kwinApp()->platform(), "setVirtualOutputs", Qt::DirectConnection, Q_ARG(int, 2));
    QVERIFY(workspaceCreatedSpy.wait());
    QCOMPARE(screens()->count(), 2);
    QCOMPARE(screens()->geometry(0), QRect(0, 0, 1280, 1024));
    QCOMPARE(screens()->geometry(1), QRect(1280, 0, 1280, 1024));
    setenv("QT_QPA_PLATFORM", "wayland", true);
    setenv("QMLSCENE_DEVICE", "softwarecontext", true);
    waylandServer()->initWorkspace();
}

void PlasmaWindowTest::init()
{
    Test::setup_wayland_connection(Test::AdditionalWaylandInterface::WindowManagement);
    m_windowManagement = Test::get_client().interfaces.window_management.get();
    m_compositor = Test::get_client().interfaces.compositor.get();

    screens()->setCurrent(0);
    input::cursor::setPos(QPoint(640, 512));
}

void PlasmaWindowTest::cleanup()
{
    Test::destroy_wayland_connection();
}

void xcb_connection_deleter(xcb_connection_t* pointer)
{
    xcb_disconnect(pointer);
}

using xcb_connection_ptr = std::unique_ptr<xcb_connection_t, void(*)(xcb_connection_t*)>;

xcb_connection_ptr create_xcb_connection()
{
    return xcb_connection_ptr(xcb_connect(nullptr, nullptr), xcb_connection_deleter);
}

void PlasmaWindowTest::testCreateDestroyX11PlasmaWindow()
{
    // this test verifies that a PlasmaWindow gets unmapped on Client side when an X11 client is destroyed
    QSignalSpy plasmaWindowCreatedSpy(m_windowManagement, &PlasmaWindowManagement::windowCreated);
    QVERIFY(plasmaWindowCreatedSpy.isValid());

    // create an xcb window
    auto c = create_xcb_connection();
    QVERIFY(!xcb_connection_has_error(c.get()));
    const QRect windowGeometry(0, 0, 100, 200);
    xcb_window_t w = xcb_generate_id(c.get());
    xcb_create_window(c.get(), XCB_COPY_FROM_PARENT, w, rootWindow(),
                      windowGeometry.x(),
                      windowGeometry.y(),
                      windowGeometry.width(),
                      windowGeometry.height(),
                      0, XCB_WINDOW_CLASS_INPUT_OUTPUT, XCB_COPY_FROM_PARENT, 0, nullptr);
    xcb_size_hints_t hints;
    memset(&hints, 0, sizeof(hints));
    xcb_icccm_size_hints_set_position(&hints, 1, windowGeometry.x(), windowGeometry.y());
    xcb_icccm_size_hints_set_size(&hints, 1, windowGeometry.width(), windowGeometry.height());
    xcb_icccm_set_wm_normal_hints(c.get(), w, &hints);
    xcb_map_window(c.get(), w);
    xcb_flush(c.get());

    // we should get a client for it
    QSignalSpy windowCreatedSpy(workspace(), &Workspace::clientAdded);
    QVERIFY(windowCreatedSpy.isValid());
    QVERIFY(windowCreatedSpy.wait());
    auto client = windowCreatedSpy.first().first().value<win::x11::window*>();
    QVERIFY(client);
    QCOMPARE(client->xcb_window(), w);
    QVERIFY(win::decoration(client));
    QVERIFY(client->control->active());
    // verify that it gets the keyboard focus
    if (!client->surface()) {
        // we don't have a surface yet, so focused keyboard surface if set is not ours
        QVERIFY(!waylandServer()->seat()->focusedKeyboardSurface());
        QSignalSpy surfaceChangedSpy(client, &Toplevel::surfaceChanged);
        QVERIFY(surfaceChangedSpy.isValid());
        QVERIFY(surfaceChangedSpy.wait());
    }
    QVERIFY(client->surface());
    QCOMPARE(waylandServer()->seat()->focusedKeyboardSurface(), client->surface());

    // now that should also give it to us on client side
    QVERIFY(plasmaWindowCreatedSpy.wait());
    QCOMPARE(plasmaWindowCreatedSpy.count(), 1);
    QCOMPARE(m_windowManagement->windows().count(), 1);
    auto pw = m_windowManagement->windows().first();
    QCOMPARE(pw->geometry(), client->frameGeometry());
    QSignalSpy geometryChangedSpy(pw, &PlasmaWindow::geometryChanged);
    QVERIFY(geometryChangedSpy.isValid());

    QSignalSpy unmappedSpy(m_windowManagement->windows().first(), &PlasmaWindow::unmapped);
    QVERIFY(unmappedSpy.isValid());
    QSignalSpy destroyedSpy(m_windowManagement->windows().first(), &QObject::destroyed);
    QVERIFY(destroyedSpy.isValid());

    // and destroy the window again
    xcb_unmap_window(c.get(), w);
    xcb_flush(c.get());

    QSignalSpy windowClosedSpy(client, &win::x11::window::windowClosed);
    QVERIFY(windowClosedSpy.isValid());
    QVERIFY(windowClosedSpy.wait());
    xcb_destroy_window(c.get(), w);
    c.reset();

    QVERIFY(unmappedSpy.count() > 1 || unmappedSpy.wait());
    QCOMPARE(unmappedSpy.count(), 2);

    QVERIFY(destroyedSpy.wait());
}

class HelperWindow : public QRasterWindow
{
    Q_OBJECT
public:
    HelperWindow();
    ~HelperWindow() override;

protected:
    void paintEvent(QPaintEvent *event) override;
};

HelperWindow::HelperWindow()
    : QRasterWindow(nullptr)
{
}

HelperWindow::~HelperWindow() = default;

void HelperWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.fillRect(0, 0, width(), height(), Qt::red);
}

void PlasmaWindowTest::testInternalWindowNoPlasmaWindow()
{
    // this test verifies that an internal window is not added as a PlasmaWindow to the client
    QSignalSpy plasmaWindowCreatedSpy(m_windowManagement, &PlasmaWindowManagement::windowCreated);
    QVERIFY(plasmaWindowCreatedSpy.isValid());
    HelperWindow win;
    win.setGeometry(0, 0, 100, 100);
    win.show();

    QVERIFY(!plasmaWindowCreatedSpy.wait(500));
}

void PlasmaWindowTest::testPopupWindowNoPlasmaWindow()
{
    // this test verifies that for a popup window no PlasmaWindow is sent to the client
    QSignalSpy plasmaWindowCreatedSpy(m_windowManagement, &PlasmaWindowManagement::windowCreated);
    QVERIFY(plasmaWindowCreatedSpy.isValid());

    // first create the parent window
    std::unique_ptr<Surface> parentSurface(Test::create_surface());
    std::unique_ptr<XdgShellToplevel> parentShellSurface(Test::create_xdg_shell_toplevel(parentSurface));
    auto parentClient = Test::render_and_wait_for_shown(parentSurface, QSize(100, 50), Qt::blue);
    QVERIFY(parentClient);
    QVERIFY(plasmaWindowCreatedSpy.wait());
    QCOMPARE(plasmaWindowCreatedSpy.count(), 1);

    // now let's create a popup window for it
    XdgPositioner positioner(QSize(10, 10), QRect(0, 0, 10, 10));
    positioner.setAnchorEdge(Qt::BottomEdge | Qt::RightEdge);
    positioner.setGravity(Qt::BottomEdge | Qt::RightEdge);
    std::unique_ptr<Surface> popupSurface(Test::create_surface());
    std::unique_ptr<XdgShellPopup> popupShellSurface(Test::create_xdg_shell_popup(popupSurface, parentShellSurface, positioner));
    auto popupClient = Test::render_and_wait_for_shown(popupSurface, positioner.initialSize(), Qt::blue);
    QVERIFY(popupClient);
    QVERIFY(!plasmaWindowCreatedSpy.wait(100));
    QCOMPARE(plasmaWindowCreatedSpy.count(), 1);

    // let's destroy the windows
    popupShellSurface.reset();
    QVERIFY(Test::wait_for_destroyed(popupClient));
    parentShellSurface.reset();
    QVERIFY(Test::wait_for_destroyed(parentClient));
}

void PlasmaWindowTest::testLockScreenNoPlasmaWindow()
{
    // this test verifies that lock screen windows are not exposed to PlasmaWindow
    QSignalSpy plasmaWindowCreatedSpy(m_windowManagement, &PlasmaWindowManagement::windowCreated);
    QVERIFY(plasmaWindowCreatedSpy.isValid());

    // this time we use a QSignalSpy on XdgShellClient as it'a a little bit more complex setup
    QSignalSpy clientAddedSpy(waylandServer(), &WaylandServer::window_added);
    QVERIFY(clientAddedSpy.isValid());

    // lock
    ScreenLocker::KSldApp::self()->lock(ScreenLocker::EstablishLock::Immediate);

    // The lock screen creates one client per screen.
    QVERIFY(clientAddedSpy.count() == screens()->count() || clientAddedSpy.wait());
    QTRY_COMPARE(clientAddedSpy.count(), screens()->count());

    QVERIFY(clientAddedSpy.first().first().value<win::wayland::window*>()->isLockScreen());

    // should not be sent to the client
    QVERIFY(plasmaWindowCreatedSpy.isEmpty());
    QVERIFY(!plasmaWindowCreatedSpy.wait(500));

    // fake unlock
    QSignalSpy lockStateChangedSpy(ScreenLocker::KSldApp::self(), &ScreenLocker::KSldApp::lockStateChanged);
    QVERIFY(lockStateChangedSpy.isValid());
    const auto children = ScreenLocker::KSldApp::self()->children();
    for (auto it = children.begin(); it != children.end(); ++it) {
        if (qstrcmp((*it)->metaObject()->className(), "LogindIntegration") != 0) {
            continue;
        }
        QMetaObject::invokeMethod(*it, "requestUnlock");
        break;
    }
    QVERIFY(lockStateChangedSpy.wait());
    QVERIFY(!waylandServer()->isScreenLocked());
}

void PlasmaWindowTest::testDestroyedButNotUnmapped()
{
    // this test verifies that also when a ShellSurface gets destroyed without a prior unmap
    // the PlasmaWindow gets destroyed on Client side
    QSignalSpy plasmaWindowCreatedSpy(m_windowManagement, &PlasmaWindowManagement::windowCreated);
    QVERIFY(plasmaWindowCreatedSpy.isValid());

    // first create the parent window
    std::unique_ptr<Surface> parentSurface(Test::create_surface());
    std::unique_ptr<XdgShellToplevel> parentShellSurface(Test::create_xdg_shell_toplevel(parentSurface));
    // map that window
    Test::render(parentSurface, QSize(100, 50), Qt::blue);
    // this should create a plasma window
    QVERIFY(plasmaWindowCreatedSpy.wait());
    QCOMPARE(plasmaWindowCreatedSpy.count(), 1);
    auto window = plasmaWindowCreatedSpy.first().first().value<PlasmaWindow*>();
    QVERIFY(window);
    QSignalSpy destroyedSpy(window, &QObject::destroyed);
    QVERIFY(destroyedSpy.isValid());

    // now destroy without an unmap
    parentShellSurface.reset();
    parentSurface.reset();
    QVERIFY(destroyedSpy.wait());
}

}

WAYLANDTEST_MAIN(KWin::PlasmaWindowTest)
#include "plasmawindow_test.moc"

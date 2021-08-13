/********************************************************************
KWin - the KDE window manager
This file is part of the KDE project.

Copyright 2016 Martin Gräßlin <mgraesslin@kde.org>
Copyright 2019 Roman Gilg <subdiff@gmail.com>

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
#include "../../xwl/databridge.h"
#include "kwin_wayland_test.h"
#include "platform.h"
#include "screens.h"
#include "wayland_server.h"
#include "workspace.h"

#include "win/wayland/window.h"
#include "win/x11/window.h"

#include <Wrapland/Server/data_device.h>
#include <Wrapland/Server/data_device_manager.h>
#include <Wrapland/Server/data_source.h>
#include <Wrapland/Server/primary_selection.h>

#include <QProcess>
#include <QProcessEnvironment>

namespace KWin
{

class XwaylandSelectionsTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanup();
    void testSync_data();
    void testSync();

private:
    QProcess* m_copyProcess = nullptr;
    QProcess* m_pasteProcess = nullptr;
};

void XwaylandSelectionsTest::initTestCase()
{
    qRegisterMetaType<win::wayland::window*>();
    qRegisterMetaType<win::x11::window*>();
    qRegisterMetaType<QProcess::ExitStatus>();
    qRegisterMetaType<Wrapland::Server::DataDevice*>();
    qRegisterMetaType<Wrapland::Server::DataSource*>();

    QSignalSpy workspaceCreatedSpy(kwinApp(), &Application::workspaceCreated);
    QVERIFY(workspaceCreatedSpy.isValid());
    kwinApp()->platform->setInitialWindowSize(QSize(1280, 1024));
    //    QSignalSpy clipboardSyncDevicedCreated{waylandServer(),
    //    &WaylandServer::xclipboardSyncDataDeviceCreated};
    //    QVERIFY(clipboardSyncDevicedCreated.isValid());

    kwinApp()->start();
    QMetaObject::invokeMethod(
        kwinApp()->platform, "setVirtualOutputs", Qt::DirectConnection, Q_ARG(int, 2));
    QVERIFY(workspaceCreatedSpy.wait());
    QCOMPARE(screens()->count(), 2);
    QCOMPARE(screens()->geometry(0), QRect(0, 0, 1280, 1024));
    QCOMPARE(screens()->geometry(1), QRect(1280, 0, 1280, 1024));
    //    // wait till the xclipboard sync data device is created
    //    if (clipboardSyncDevicedCreated.empty()) {
    //        QVERIFY(clipboardSyncDevicedCreated.wait());
    //    }
    // wait till the DataBridge sync data device is created
    while (Xwl::DataBridge::self()->dataDeviceIface() == nullptr) {
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }
    QVERIFY(Xwl::DataBridge::self()->dataDeviceIface() != nullptr);
}

void XwaylandSelectionsTest::cleanup()
{
    if (m_copyProcess) {
        m_copyProcess->terminate();
        QVERIFY(m_copyProcess->waitForFinished());
        m_copyProcess = nullptr;
    }
    if (m_pasteProcess) {
        m_pasteProcess->terminate();
        QVERIFY(m_pasteProcess->waitForFinished());
        m_pasteProcess = nullptr;
    }
}

void XwaylandSelectionsTest::testSync_data()
{
    QTest::addColumn<QString>("clipboardMode");
    QTest::addColumn<QString>("copyPlatform");
    QTest::addColumn<QString>("pastePlatform");

    QTest::newRow("Clipboard x11->wayland")
        << QStringLiteral("Clipboard") << QStringLiteral("xcb") << QStringLiteral("wayland");
    QTest::newRow("Clipboard wayland->x11")
        << QStringLiteral("Clipboard") << QStringLiteral("wayland") << QStringLiteral("xcb");
    QTest::newRow("primary_selection x11->wayland")
        << QStringLiteral("Selection") << QStringLiteral("xcb") << QStringLiteral("wayland");
    QTest::newRow("primary_selection wayland->x11")
        << QStringLiteral("Selection") << QStringLiteral("wayland") << QStringLiteral("xcb");
}

void XwaylandSelectionsTest::testSync()
{
    QFETCH(QString, clipboardMode);
    if (clipboardMode == "Clipboard") {
        QVERIFY(Xwl::DataBridge::self()->dataDeviceIface() != nullptr);
    }
    if (clipboardMode == "Selection") {
        QVERIFY(Xwl::DataBridge::self()->primarySelectionDeviceIface() != nullptr);
    }

    // this test verifies the syncing of X11 to Wayland clipboard
    const QString copy = QFINDTESTDATA(QStringLiteral("copy"));
    QVERIFY(!copy.isEmpty());
    const QString paste = QFINDTESTDATA(QStringLiteral("paste"));
    QVERIFY(!paste.isEmpty());

    QSignalSpy clientAddedSpy(workspace(), &Workspace::clientAdded);
    QVERIFY(clientAddedSpy.isValid());
    QSignalSpy shellClientAddedSpy(waylandServer(), &WaylandServer::window_added);
    QVERIFY(shellClientAddedSpy.isValid());

    QSignalSpy clipboardChangedSpy = [clipboardMode]() {
        if (clipboardMode == "Clipboard") {
            return QSignalSpy(Xwl::DataBridge::self()->dataDeviceIface(),
                              &Wrapland::Server::DataDevice::selectionChanged);
        }
        if (clipboardMode == "Selection") {
            return QSignalSpy(Xwl::DataBridge::self()->primarySelectionDeviceIface(),
                              &Wrapland::Server::PrimarySelectionDevice::selectionChanged);
        }
        throw;
    }();
    QVERIFY(clipboardChangedSpy.isValid());

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();

    // start the copy process
    QFETCH(QString, copyPlatform);
    environment.insert(QStringLiteral("QT_QPA_PLATFORM"), copyPlatform);
    m_copyProcess = new QProcess();
    m_copyProcess->setProcessEnvironment(environment);
    m_copyProcess->setProcessChannelMode(QProcess::ForwardedChannels);
    m_copyProcess->setProgram(copy);
    m_copyProcess->setArguments({clipboardMode});
    m_copyProcess->start();
    QVERIFY(m_copyProcess->waitForStarted());

    Toplevel* copyClient = nullptr;
    if (copyPlatform == QLatin1String("xcb")) {
        QVERIFY(clientAddedSpy.wait());
        copyClient = clientAddedSpy.first().first().value<Toplevel*>();
    } else {
        QVERIFY(shellClientAddedSpy.wait());
        copyClient = shellClientAddedSpy.first().first().value<Toplevel*>();
    }
    QVERIFY(copyClient);
    if (workspace()->activeClient() != copyClient) {
        workspace()->activateClient(copyClient);
    }
    QCOMPARE(workspace()->activeClient(), copyClient);
    if (copyPlatform == QLatin1String("xcb")) {
        QVERIFY(clipboardChangedSpy.isEmpty());
        QVERIFY(clipboardChangedSpy.wait());
    } else {
        // TODO: it would be better to be able to connect to a signal, instead of waiting
        // the idea is to make sure that the clipboard is updated, thus we need to give it
        // enough time before starting the paste process which creates another window
        QTest::qWait(250);
    }

    // start the paste process
    m_pasteProcess = new QProcess();
    QSignalSpy finishedSpy(
        m_pasteProcess,
        static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished));
    QVERIFY(finishedSpy.isValid());
    QFETCH(QString, pastePlatform);
    environment.insert(QStringLiteral("QT_QPA_PLATFORM"), pastePlatform);
    m_pasteProcess->setProcessEnvironment(environment);
    m_pasteProcess->setProcessChannelMode(QProcess::ForwardedChannels);
    m_pasteProcess->setProgram(paste);
    m_pasteProcess->setArguments({clipboardMode});
    m_pasteProcess->start();
    QVERIFY(m_pasteProcess->waitForStarted());

    Toplevel* pasteClient = nullptr;
    if (pastePlatform == QLatin1String("xcb")) {
        QVERIFY(clientAddedSpy.wait());
        pasteClient = clientAddedSpy.last().first().value<Toplevel*>();
    } else {
        QVERIFY(shellClientAddedSpy.wait());
        pasteClient = shellClientAddedSpy.last().first().value<Toplevel*>();
    }
    QCOMPARE(clientAddedSpy.count(), 1);
    QCOMPARE(shellClientAddedSpy.count(), 1);
    QVERIFY(pasteClient);

    if (workspace()->activeClient() != pasteClient) {
        QSignalSpy clientActivatedSpy(workspace(), &Workspace::clientActivated);
        QVERIFY(clientActivatedSpy.isValid());
        workspace()->activateClient(pasteClient);
        QVERIFY(clientActivatedSpy.wait());
    }
    QTRY_COMPARE(workspace()->activeClient(), pasteClient);
    QVERIFY(finishedSpy.wait());
    QCOMPARE(finishedSpy.first().first().toInt(), 0);
    delete m_pasteProcess;
    m_pasteProcess = nullptr;
    delete m_copyProcess;
    m_copyProcess = nullptr;
}

}

WAYLANDTEST_MAIN(KWin::XwaylandSelectionsTest)
#include "xwayland_selections_test.moc"

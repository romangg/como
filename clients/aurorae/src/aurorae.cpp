/********************************************************************
Copyright (C) 2009, 2010, 2012 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#include "aurorae.h"
#include "auroraetheme.h"
#include "config-kwin.h"
// qml imports
#include "decorationoptions.h"
// KDecoration2
#include <KDecoration2/DecoratedClient>
#include <KDecoration2/DecorationSettings>
#include <KDecoration2/DecorationShadow>
// KDE
#include <KConfigGroup>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KService>
#include <KServiceTypeTrader>
// Qt
#include <QDebug>
#include <QDirIterator>
#include <QOpenGLFramebufferObject>
#include <QPainter>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QStandardPaths>

K_PLUGIN_FACTORY_WITH_JSON(AuroraeDecoFactory,
                           "aurorae.json",
                           registerPlugin<Aurorae::Decoration>();)

namespace Aurorae
{

class Helper
{
public:
    void ref();
    void unref();
    QQmlComponent *component(const QString &theme);
    QQmlContext *rootContext();
    QQmlComponent *svgComponent() {
        return m_svgComponent.data();
    }

    static Helper &instance();
private:
    Helper() = default;
    void init();
    QQmlComponent *loadComponent(const QString &themeName);
    int m_refCount = 0;
    QScopedPointer<QQmlEngine> m_engine;
    QHash<QString, QQmlComponent*> m_components;
    QScopedPointer<QQmlComponent> m_svgComponent;
};

Helper &Helper::instance()
{
    static Helper s_helper;
    return s_helper;
}

void Helper::ref()
{
    m_refCount++;
    if (m_refCount == 1) {
        m_engine.reset(new QQmlEngine);
        init();
    }
}

void Helper::unref()
{
    m_refCount--;
    if (m_refCount == 0) {
        // cleanup
        m_svgComponent.reset();
        m_engine.reset();
        m_components.clear();
    }
}

static const QString s_defaultTheme = QStringLiteral("kwin4_decoration_qml_plastik");

QQmlComponent *Helper::component(const QString &themeName)
{
    // maybe it's an SVG theme?
    if (themeName.startsWith(QLatin1Literal("__aurorae__svg__"))) {
        if (m_svgComponent.isNull()) {
            /* use logic from KDeclarative::setupBindings():
            "addImportPath adds the path at the beginning, so to honour user's
            paths we need to traverse the list in reverse order" */
            QStringListIterator paths(QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("module/imports"), QStandardPaths::LocateDirectory));
            paths.toBack();
            while (paths.hasPrevious()) {
                m_engine->addImportPath(paths.previous());
            }
            m_svgComponent.reset(new QQmlComponent(m_engine.data()));
            m_svgComponent->loadUrl(QUrl(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kwin/aurorae/aurorae.qml"))));
        }
        return m_svgComponent.data();
    }
    // try finding the QML package
    auto it = m_components.constFind(themeName);
    if (it != m_components.constEnd()) {
        return it.value();
    }
    auto component = loadComponent(themeName);
    if (component) {
        m_components.insert(themeName, component);
        return component;
    }
    // try loading default component
    if (themeName != s_defaultTheme) {
        return loadComponent(s_defaultTheme);
    }
    return nullptr;
}

QQmlComponent *Helper::loadComponent(const QString &themeName)
{
    qCDebug(AURORAE) << "Trying to load QML Decoration " << themeName;
    const QString internalname = themeName.toLower();

    QString constraint = QStringLiteral("[X-KDE-PluginInfo-Name] == '%1'").arg(internalname);
    KService::List offers = KServiceTypeTrader::self()->query(QStringLiteral("KWin/Decoration"), constraint);
    if (offers.isEmpty()) {
        qCCritical(AURORAE) << "Couldn't find QML Decoration " << themeName << endl;
        // TODO: what to do in error case?
        return nullptr;
    }
    KService::Ptr service = offers.first();
    const QString pluginName = service->property(QStringLiteral("X-KDE-PluginInfo-Name")).toString();
    const QString scriptName = service->property(QStringLiteral("X-Plasma-MainScript")).toString();
    const QString file = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral(KWIN_NAME) + QStringLiteral("/decorations/") + pluginName + QStringLiteral("/contents/") + scriptName);
    if (file.isNull()) {
        qCDebug(AURORAE) << "Could not find script file for " << pluginName;
        // TODO: what to do in error case?
        return nullptr;
    }
    // setup the QML engine
    /* use logic from KDeclarative::setupBindings():
    "addImportPath adds the path at the beginning, so to honour user's
     paths we need to traverse the list in reverse order" */
    QStringListIterator paths(QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("module/imports"), QStandardPaths::LocateDirectory));
    paths.toBack();
    while (paths.hasPrevious()) {
        m_engine->addImportPath(paths.previous());
    }
    QQmlComponent *component = new QQmlComponent(m_engine.data(), m_engine.data());
    component->loadUrl(QUrl::fromLocalFile(file));
    return component;
}

QQmlContext *Helper::rootContext()
{
    return m_engine->rootContext();
}

void Helper::init()
{
    // we need to first load our decoration plugin
    // once it's loaded we can provide the Borders and access them from C++ side
    // so let's try to locate our plugin:
    QString pluginPath;
    for (const QString &path : m_engine->importPathList()) {
        QDirIterator it(path, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            QFileInfo fileInfo = it.fileInfo();
            if (!fileInfo.isFile()) {
                continue;
            }
            if (!fileInfo.path().endsWith(QLatin1String("/org/kde/kwin/decoration"))) {
                continue;
            }
            if (fileInfo.fileName() == QLatin1String("libdecorationplugin.so")) {
                pluginPath = fileInfo.absoluteFilePath();
                break;
            }
        }
        if (!pluginPath.isEmpty()) {
            break;
        }
    }
    m_engine->importPlugin(pluginPath, "org.kde.kwin.decoration", nullptr);
    qmlRegisterType<KWin::Borders>("org.kde.kwin.decoration", 0, 1, "Borders");

    qmlRegisterType<KDecoration2::Decoration>();
    qmlRegisterType<KDecoration2::DecoratedClient>();
}


Decoration::Decoration(QObject *parent, const QVariantList &args)
    : KDecoration2::Decoration(parent)
    , m_item(nullptr)
    , m_borders(nullptr)
    , m_maximizedBorders(nullptr)
    , m_extendedBorders(nullptr)
    , m_padding(nullptr)
    , m_themeName(s_defaultTheme)
    , m_mutex(QMutex::Recursive)
{
    if (!args.isEmpty()) {
        const auto map = args.first().toMap();
        auto it = map.constFind(QStringLiteral("theme"));
        if (it != map.constEnd()) {
            m_themeName = it.value().toString();
        }
    }
    Helper::instance().ref();
    // recreate scene when compositing gets disabled, TODO: remove with rendercontrol
    connect(KDecoration2::DecorationSettings::self(), &KDecoration2::DecorationSettings::alphaChannelSupportedChanged,
            this, [this](bool alpha) {
                if (!alpha && m_item) {
                    m_item->deleteLater();
                    m_decorationWindow.reset();
                    init();
                }
            });
}

Decoration::~Decoration()
{
    Helper::instance().unref();
}

void Decoration::init()
{
    KDecoration2::Decoration::init();
    QQmlContext *context = new QQmlContext(Helper::instance().rootContext(), this);
    context->setContextProperty(QStringLiteral("decoration"), this);
    auto component = Helper::instance().component(m_themeName);
    if (!component) {
        return;
    }
    if (component == Helper::instance().svgComponent()) {
        // load SVG theme
        const QString themeName = m_themeName.mid(16);
        KConfig config(QStringLiteral("aurorae/themes/") + themeName + QStringLiteral("/") + themeName + QStringLiteral("rc"),
                       KConfig::FullConfig, QStandardPaths::GenericDataLocation);
//         KConfigGroup themeGroup(&conf, themeName);
        AuroraeTheme *theme = new AuroraeTheme(this);
        theme->loadTheme(themeName, config);
//         m_theme->setBorderSize((KDecorationDefines::BorderSize)themeGroup.readEntry<int>("BorderSize", KDecorationDefines::BorderNormal));
//         m_theme->setButtonSize((KDecorationDefines::BorderSize)themeGroup.readEntry<int>("ButtonSize", KDecorationDefines::BorderNormal));
//         m_theme->setTabDragMimeType(tabDragMimeType());
        context->setContextProperty(QStringLiteral("auroraeTheme"), theme);
    }
    m_item = qobject_cast< QQuickItem* >(component->create(context));
    if (!m_item) {
        return;
    }
    m_item->setParent(this);

    QVariant visualParent = property("visualParent");
    if (visualParent.isValid()) {
        m_item->setParentItem(visualParent.value<QQuickItem*>());
    } else {
        // we need a QQuickWindow till we depend on Qt 5.4
        m_decorationWindow.reset(QWindow::fromWinId(client()->decorationId()));
        m_view = new QQuickWindow(m_decorationWindow.data());
        m_view->setFlags(Qt::WindowDoesNotAcceptFocus | Qt::WindowTransparentForInput);
        m_view->setColor(Qt::transparent);
        connect(m_view.data(), &QQuickWindow::beforeRendering, [this]() {
            if (!KDecoration2::DecorationSettings::self()->isAlphaChannelSupported()) {
                // directly render to QQuickWindow
                m_fbo.reset();
                return;
            }
            if (m_fbo.isNull() || m_fbo->size() != m_view->size()) {
                m_fbo.reset(new QOpenGLFramebufferObject(m_view->size(), QOpenGLFramebufferObject::CombinedDepthStencil));
                if (!m_fbo->isValid()) {
                    qCWarning(AURORAE) << "Creating FBO as render target failed";
                    m_fbo.reset();
                    return;
                }
            }
            m_view->setRenderTarget(m_fbo.data());
        });
        connect(m_view.data(), &QQuickWindow::afterRendering, [this] {
            if (!m_fbo) {
                return;
            }
            QMutexLocker locker(&m_mutex);
            m_buffer = m_fbo->toImage();
        });
        connect(KDecoration2::DecorationSettings::self(), &KDecoration2::DecorationSettings::alphaChannelSupportedChanged,
                m_view.data(), &QQuickWindow::update);
        connect(m_view.data(), &QQuickWindow::afterRendering, this, [this] { update(); }, Qt::QueuedConnection);
        m_item->setParentItem(m_view->contentItem());
    }
    setupBorders(m_item);
    if (m_extendedBorders) {
        auto updateExtendedBorders = [this] {
            setExtendedBorders(m_extendedBorders->left(), m_extendedBorders->right(), m_extendedBorders->top(), m_extendedBorders->bottom());
        };
        updateExtendedBorders();
        connect(m_extendedBorders, &KWin::Borders::leftChanged, this, updateExtendedBorders);
        connect(m_extendedBorders, &KWin::Borders::rightChanged, this, updateExtendedBorders);
        connect(m_extendedBorders, &KWin::Borders::topChanged, this, updateExtendedBorders);
        connect(m_extendedBorders, &KWin::Borders::bottomChanged, this, updateExtendedBorders);
    }
    connect(client().data(), &KDecoration2::DecoratedClient::maximizedChanged, this, &Decoration::updateBorders, Qt::QueuedConnection);
    updateBorders();
    if (!m_view.isNull()) {
        m_view->setVisible(true);
        auto resizeWindow = [this] {
            QRect rect(QPoint(0, 0), size());
            if (m_padding && !client()->isMaximized()) {
                rect = rect.adjusted(-m_padding->left(), -m_padding->top(), m_padding->right(), m_padding->bottom());
            }
            m_view->setGeometry(rect);
            m_view->lower();
            m_view->update();
        };
        connect(this, &Decoration::bordersChanged, this, resizeWindow);
        connect(client().data(), &KDecoration2::DecoratedClient::widthChanged, this, resizeWindow);
        connect(client().data(), &KDecoration2::DecoratedClient::heightChanged, this, resizeWindow);
        connect(client().data(), &KDecoration2::DecoratedClient::maximizedChanged, this, resizeWindow);
        resizeWindow();
    }
}

QVariant Decoration::readConfig(const QString &key, const QVariant &defaultValue)
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("auroraerc"));
    return config->group(QStringLiteral("Plastik")).readEntry(key, defaultValue);
}

void Decoration::setupBorders(QQuickItem *item)
{
    m_borders          = item->findChild<KWin::Borders*>(QStringLiteral("borders"));
    m_maximizedBorders = item->findChild<KWin::Borders*>(QStringLiteral("maximizedBorders"));
    m_extendedBorders  = item->findChild<KWin::Borders*>(QStringLiteral("extendedBorders"));
    m_padding          = item->findChild<KWin::Borders*>(QStringLiteral("padding"));
}

void Decoration::updateBorders()
{
    KWin::Borders *b = m_borders;
    if (client()->isMaximized() && m_maximizedBorders) {
        b = m_maximizedBorders;
    }
    if (!b) {
        return;
    }
    setBorders(b->left(), b->right(), b->top(), b->bottom());
}

void Decoration::paint(QPainter *painter)
{
    if (!KDecoration2::DecorationSettings::self()->isAlphaChannelSupported()) {
        return;
    }
    QMutexLocker locker(&m_mutex);
    painter->fillRect(rect(), Qt::transparent);
    QRectF r(QPointF(0, 0), m_buffer.size());
    if (m_padding &&
            (m_padding->left() > 0 || m_padding->top() > 0 || m_padding->right() > 0 || m_padding->bottom() > 0) &&
            !client()->isMaximized()) {
        r = r.adjusted(m_padding->left(), m_padding->top(), -m_padding->right(), -m_padding->bottom());
        KDecoration2::DecorationShadow *s = new KDecoration2::DecorationShadow(this);
        s->setShadow(m_buffer);
        s->setPaddingLeft(m_padding->left());
        s->setPaddingTop(m_padding->top());
        s->setPaddingRight(m_padding->right());
        s->setPaddingBottom(m_padding->bottom());
        s->setTopLeft(QSize(m_padding->left(), m_padding->top()));
        s->setTopRight(QSize(m_padding->right(), m_padding->top()));
        s->setBottomLeft(QSize(m_padding->left(), m_padding->bottom()));
        s->setBottomRight(QSize(m_padding->right(), m_padding->bottom()));
        s->setLeft(QSize(m_padding->left(), m_buffer.height() - m_padding->top() - m_padding->bottom()));
        s->setRight(QSize(m_padding->right(), m_buffer.height() - m_padding->top() - m_padding->bottom()));
        s->setTop(QSize(m_buffer.width() - m_padding->left() - m_padding->right(), m_padding->top()));
        s->setBottom(QSize(m_buffer.width() - m_padding->left() - m_padding->right(), m_padding->bottom()));
        auto oldShadow = shadow();
        setShadow(s);
        if (oldShadow) {
            delete oldShadow.data();
        }
    } else {
        setShadow(QPointer<KDecoration2::DecorationShadow>());
    }
    painter->drawImage(rect(), m_buffer, r);
}

QMouseEvent Decoration::translatedMouseEvent(QMouseEvent *orig)
{
    if (!m_padding || client()->isMaximized()) {
        orig->setAccepted(false);
        return *orig;
    }
    QMouseEvent event(orig->type(), orig->localPos() + QPointF(m_padding->left(), m_padding->top()), orig->button(), orig->buttons(), orig->modifiers());
    event.setAccepted(false);
    return event;
}

void Decoration::hoverEnterEvent(QHoverEvent *event)
{
    if (m_view) {
        event->setAccepted(false);
        QCoreApplication::sendEvent(m_view.data(), event);
    }
    KDecoration2::Decoration::hoverEnterEvent(event);
}

void Decoration::hoverLeaveEvent(QHoverEvent *event)
{
    if (m_view) {
        event->setAccepted(false);
        QCoreApplication::sendEvent(m_view.data(), event);
    }
    KDecoration2::Decoration::hoverLeaveEvent(event);
}

void Decoration::hoverMoveEvent(QHoverEvent *event)
{
    if (m_view) {
        QMouseEvent mouseEvent(QEvent::MouseMove, event->posF(), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QMouseEvent ev = translatedMouseEvent(&mouseEvent);
        QCoreApplication::sendEvent(m_view.data(), &ev);
        event->setAccepted(ev.isAccepted());
    }
    KDecoration2::Decoration::hoverMoveEvent(event);
}

void Decoration::mouseMoveEvent(QMouseEvent *event)
{
    if (m_view) {
        QMouseEvent ev = translatedMouseEvent(event);
        QCoreApplication::sendEvent(m_view.data(), &ev);
        event->setAccepted(ev.isAccepted());
    }
    KDecoration2::Decoration::mouseMoveEvent(event);
}

void Decoration::mousePressEvent(QMouseEvent *event)
{
    if (m_view) {
        QMouseEvent ev = translatedMouseEvent(event);
        QCoreApplication::sendEvent(m_view.data(), &ev);
        event->setAccepted(ev.isAccepted());
    }
    KDecoration2::Decoration::mousePressEvent(event);
}

void Decoration::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_view) {
        QMouseEvent ev = translatedMouseEvent(event);
        QCoreApplication::sendEvent(m_view.data(), &ev);
        event->setAccepted(ev.isAccepted());
    }
    KDecoration2::Decoration::mouseReleaseEvent(event);
}

void Decoration::installTitleItem(QQuickItem *item)
{
    auto update = [this, item] {
        QRect rect = item->mapRectToScene(item->childrenRect()).toRect();
        if (rect.isNull()) {
            rect = item->parentItem()->mapRectToScene(QRectF(item->x(), item->y(), item->width(), item->height())).toRect();
        }
        setTitleRect(rect);
    };
    update();
    connect(item, &QQuickItem::widthChanged, this, update);
    connect(item, &QQuickItem::heightChanged, this, update);
    connect(item, &QQuickItem::xChanged, this, update);
    connect(item, &QQuickItem::yChanged, this, update);
}

}

#include "aurorae.moc"

/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2013 Martin Gräßlin <mgraesslin@kde.org>

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
#include "scene.h"

#include "effect_frame.h"
#include "shadow.h"
#include "window.h"

#include "base/output.h"
#include "input/cursor.h"
#include "main.h"
#include "platform.h"
#include "render/compositor.h"
#include "render/cursor.h"
#include "screens.h"
#include "toplevel.h"

#include <kwineffectquickview.h>

#include <Wrapland/Server/buffer.h>
#include <Wrapland/Server/surface.h>

#include "decorations/decoratedclient.h"

#include <KDecoration2/Decoration>
#include <QPainter>

#include <cmath>

namespace KWin::render::qpainter
{

scene::scene(qpainter::backend* backend)
    : m_backend(backend)
    , m_painter(new QPainter())
{
}

scene::~scene()
{
}

CompositingType scene::compositingType() const
{
    return QPainterCompositing;
}

bool scene::initFailed() const
{
    return false;
}

void scene::paintGenericScreen(paint_type mask, ScreenPaintData data)
{
    m_painter->save();
    m_painter->translate(data.xTranslation(), data.yTranslation());
    m_painter->scale(data.xScale(), data.yScale());
    render::scene::paintGenericScreen(mask, data);
    m_painter->restore();
}

int64_t scene::paint_output(base::output* output,
                            QRegion damage,
                            std::deque<Toplevel*> const& toplevels,
                            std::chrono::milliseconds presentTime)
{
    QElapsedTimer renderTimer;
    renderTimer.start();

    createStackingOrder(toplevels);

    auto mask = paint_type::none;
    m_backend->prepareRenderingFrame();

    auto const needsFullRepaint = m_backend->needsFullRepaint();
    if (needsFullRepaint) {
        mask |= render::paint_type::screen_background_first;
        damage = screens()->geometry();
    }

    auto const geometry = output->geometry();

    auto buffer = m_backend->bufferForScreen(output);
    if (!buffer || buffer->isNull()) {
        return renderTimer.nsecsElapsed();
    }

    m_painter->begin(buffer);
    m_painter->save();
    m_painter->setWindow(geometry);

    repaint_output = output;
    QRegion updateRegion, validRegion;

    paintScreen(
        mask, damage.intersected(geometry), QRegion(), &updateRegion, &validRegion, presentTime);
    paintCursor();

    m_painter->restore();
    m_painter->end();

    m_backend->present(output, updateRegion);

    clearStackingOrder();
    Q_EMIT frameRendered();

    return renderTimer.nsecsElapsed();
}

void scene::paintBackground(QRegion region)
{
    m_painter->setBrush(Qt::black);
    for (const QRect& rect : region) {
        m_painter->drawRect(rect);
    }
}

void scene::paintCursor()
{
    auto cursor = render::compositor::self()->software_cursor.get();
    if (!cursor->enabled) {
        return;
    }
    auto const img = cursor->image();
    if (img.isNull()) {
        return;
    }
    auto const cursorPos = input::get_cursor()->pos();
    auto const hotspot = cursor->hotspot();
    m_painter->drawImage(cursorPos - hotspot, img);
    cursor->mark_as_rendered();
}

void scene::paintEffectQuickView(EffectQuickView* w)
{
    QPainter* painter = effects->scenePainter();
    const QImage buffer = w->bufferAsImage();
    if (buffer.isNull()) {
        return;
    }
    painter->drawImage(w->geometry(), buffer);
}

render::window* scene::createWindow(Toplevel* toplevel)
{
    return new window(this, toplevel);
}

render::effect_frame* scene::createEffectFrame(effect_frame_impl* frame)
{
    return new effect_frame(frame, this);
}

render::shadow* scene::createShadow(Toplevel* toplevel)
{
    return new shadow(toplevel);
}

Decoration::Renderer* scene::createDecorationRenderer(Decoration::DecoratedClientImpl* impl)
{
    return new deco_renderer(impl);
}

void scene::handle_screen_geometry_change(QSize const& size)
{
    m_backend->screenGeometryChanged(size);
}

QImage* scene::qpainterRenderBuffer() const
{
    return m_backend->buffer();
}

//****************************************
// QPainterDecorationRenderer
//****************************************
deco_renderer::deco_renderer(Decoration::DecoratedClientImpl* client)
    : Renderer(client)
{
    connect(this,
            &Renderer::renderScheduled,
            client->client(),
            static_cast<void (Toplevel::*)(QRegion const&)>(&Toplevel::addRepaint));
}

deco_renderer::~deco_renderer() = default;

QImage deco_renderer::image(deco_renderer::DecorationPart part) const
{
    Q_ASSERT(part != DecorationPart::Count);
    return m_images[int(part)];
}

void deco_renderer::render()
{
    const QRegion scheduled = getScheduled();
    if (scheduled.isEmpty()) {
        return;
    }
    if (areImageSizesDirty()) {
        resizeImages();
        resetImageSizesDirty();
    }

    auto imageSize = [this](DecorationPart part) {
        return m_images[int(part)].size() / m_images[int(part)].devicePixelRatio();
    };

    const QRect top(QPoint(0, 0), imageSize(DecorationPart::Top));
    const QRect left(QPoint(0, top.height()), imageSize(DecorationPart::Left));
    const QRect right(QPoint(top.width() - imageSize(DecorationPart::Right).width(), top.height()),
                      imageSize(DecorationPart::Right));
    const QRect bottom(QPoint(0, left.y() + left.height()), imageSize(DecorationPart::Bottom));

    const QRect geometry = scheduled.boundingRect();
    auto renderPart = [this](const QRect& rect, const QRect& partRect, int index) {
        if (rect.isEmpty()) {
            return;
        }
        QPainter painter(&m_images[index]);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setWindow(
            QRect(partRect.topLeft(), partRect.size() * m_images[index].devicePixelRatio()));
        painter.setClipRect(rect);
        painter.save();
        // clear existing part
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(rect, Qt::transparent);
        painter.restore();
        client()->decoration()->paint(&painter, rect);
    };

    renderPart(left.intersected(geometry), left, int(DecorationPart::Left));
    renderPart(top.intersected(geometry), top, int(DecorationPart::Top));
    renderPart(right.intersected(geometry), right, int(DecorationPart::Right));
    renderPart(bottom.intersected(geometry), bottom, int(DecorationPart::Bottom));
}

void deco_renderer::resizeImages()
{
    QRect left, top, right, bottom;
    client()->client()->layoutDecorationRects(left, top, right, bottom);

    auto checkAndCreate = [this](int index, const QSize& size) {
        auto dpr = client()->client()->screenScale();
        if (m_images[index].size() != size * dpr || m_images[index].devicePixelRatio() != dpr) {
            m_images[index] = QImage(size * dpr, QImage::Format_ARGB32_Premultiplied);
            m_images[index].setDevicePixelRatio(dpr);
            m_images[index].fill(Qt::transparent);
        }
    };
    checkAndCreate(int(DecorationPart::Left), left.size());
    checkAndCreate(int(DecorationPart::Right), right.size());
    checkAndCreate(int(DecorationPart::Top), top.size());
    checkAndCreate(int(DecorationPart::Bottom), bottom.size());
}

void deco_renderer::reparent(Toplevel* window)
{
    render();
    Renderer::reparent(window);
}

render::scene* create_scene()
{
    QScopedPointer<qpainter::backend> backend(kwinApp()->platform->createQPainterBackend());
    if (backend.isNull()) {
        return nullptr;
    }
    if (backend->isFailed()) {
        return nullptr;
    }

    auto s = new scene(backend.take());

    if (s && s->initFailed()) {
        delete s;
        s = nullptr;
    }
    return s;
}

}

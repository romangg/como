/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2011 Arthur Arlt <a.arlt@stud.uni-heidelberg.de>

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
#pragma once

#include <kwinglobals.h>

#include <QObject>
#include <QRect>

#include <kwin_export.h>

class QQmlContext;
class QQmlComponent;

namespace KWin::render::x11
{

class outline_visual;

/**
 * @short This class is used to show the outline of a given geometry.
 *
 * The class renders an outline by using four windows. One for each border of
 * the geometry. It is possible to replace the outline with an effect. If an
 * effect is available the effect will be used, otherwise the outline will be
 * rendered by using the X implementation.
 *
 * @author Arthur Arlt
 * @since 4.7
 */
class KWIN_EXPORT outline : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QRect geometry READ geometry NOTIFY geometryChanged)
    Q_PROPERTY(
        QRect visualParentGeometry READ visualParentGeometry NOTIFY visualParentGeometryChanged)
    Q_PROPERTY(QRect unifiedGeometry READ unifiedGeometry NOTIFY unifiedGeometryChanged)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
public:
    ~outline() override;

    /**
     * Set the outline geometry.
     * To show the outline use showOutline.
     * @param outlineGeometry The geometry of the outline to be shown
     * @see showOutline
     */
    void setGeometry(const QRect& outlineGeometry);

    /**
     * Set the visual parent geometry.
     * This is the geometry from which the will emerge.
     * @param visualParentGeometry The visual geometry of the visual parent
     * @see showOutline
     */
    void setVisualParentGeometry(const QRect& visualParentGeometry);

    /**
     * Shows the outline of a window using either an effect or the X implementation.
     * To stop the outline process use hideOutline.
     * @see hideOutline
     */
    void show();

    /**
     * Shows the outline for the given @p outlineGeometry.
     * This is the same as setOutlineGeometry followed by showOutline directly.
     * To stop the outline process use hideOutline.
     * @param outlineGeometry The geometry of the outline to be shown
     * @see hideOutline
     */
    void show(const QRect& outlineGeometry);

    /**
     * Shows the outline for the given @p outlineGeometry animated from @p visualParentGeometry.
     * This is the same as setOutlineGeometry followed by setVisualParentGeometry
     * and then showOutline.
     * To stop the outline process use hideOutline.
     * @param outlineGeometry The geometry of the outline to be shown
     * @param visualParentGeometry The geometry from where the outline should emerge
     * @see hideOutline
     * @since 5.10
     */
    void show(const QRect& outlineGeometry, const QRect& visualParentGeometry);

    /**
     * Hides shown outline.
     * @see showOutline
     */
    void hide();

    const QRect& geometry() const;
    const QRect& visualParentGeometry() const;
    QRect unifiedGeometry() const;

    bool isActive() const;

private Q_SLOTS:
    void compositingChanged();

Q_SIGNALS:
    void activeChanged();
    void geometryChanged();
    void unifiedGeometryChanged();
    void visualParentGeometryChanged();

private:
    void createHelper();
    QScopedPointer<outline_visual> m_visual;
    QRect m_outlineGeometry;
    QRect m_visualParentGeometry;
    bool m_active;

    KWIN_SINGLETON(outline)
};

class KWIN_EXPORT outline_visual
{
public:
    outline_visual(x11::outline* outline);
    virtual ~outline_visual();
    virtual void show() = 0;
    virtual void hide() = 0;

protected:
    x11::outline* get_outline();
    x11::outline const* get_outline() const;

private:
    x11::outline* m_outline;
};

class composited_outline_visual : public outline_visual
{
public:
    composited_outline_visual(x11::outline* outline);
    ~composited_outline_visual() override;
    void show() override;
    void hide() override;

private:
    QScopedPointer<QQmlContext> m_qmlContext;
    QScopedPointer<QQmlComponent> m_qmlComponent;
    QScopedPointer<QObject> m_mainItem;
};

inline bool outline::isActive() const
{
    return m_active;
}

inline const QRect& outline::geometry() const
{
    return m_outlineGeometry;
}

inline const QRect& outline::visualParentGeometry() const
{
    return m_visualParentGeometry;
}

inline x11::outline* outline_visual::get_outline()
{
    return m_outline;
}

inline const x11::outline* outline_visual::get_outline() const
{
    return m_outline;
}

inline x11::outline* get_outline()
{
    return x11::outline::self();
}

}

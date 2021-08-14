/*
    SPDX-FileCopyrightText: 2013 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2018 Roman Gilg <subdiff@gmail.com>
    SPDX-FileCopyrightText: 2019 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <kwin_export.h>

#include <QPointer>
#include <QWindow>

namespace KWin
{
class Toplevel;

namespace Decoration
{
class DecoratedClientImpl;
}

namespace input
{
class redirect;

class KWIN_EXPORT device_redirect : public QObject
{
    Q_OBJECT
public:
    ~device_redirect() override;
    virtual void init();

    void update();

    /**
     * @brief First Toplevel currently at the position of the input device
     * according to the stacking order.
     * @return Toplevel* at device position.
     *
     * This will be null if no toplevel is at the position
     */
    Toplevel* at() const;
    /**
     * @brief Toplevel currently having pointer input focus (this might
     * be different from the Toplevel at the position of the pointer).
     * @return Toplevel* with pointer focus.
     *
     * This will be null if no toplevel has focus
     */
    Toplevel* focus() const;

    /**
     * @brief The Decoration currently receiving events.
     * @return decoration with pointer focus.
     */
    Decoration::DecoratedClientImpl* decoration() const;
    /**
     * @brief The internal window currently receiving events.
     * @return QWindow with pointer focus.
     */
    QWindow* internalWindow() const;

    virtual QPointF position() const = 0;

    void setFocus(Toplevel* toplevel);
    void setDecoration(Decoration::DecoratedClientImpl* decoration);
    void setInternalWindow(QWindow* window);

Q_SIGNALS:
    void decorationChanged();

protected:
    explicit device_redirect(redirect* parent);

    virtual void cleanupInternalWindow(QWindow* old, QWindow* now) = 0;
    virtual void cleanupDecoration(Decoration::DecoratedClientImpl* old,
                                   Decoration::DecoratedClientImpl* now)
        = 0;

    virtual void focusUpdate(Toplevel* old, Toplevel* now) = 0;

    /**
     * Certain input devices can be in a state of having no valid
     * position. An example are touch screens when no finger/pen
     * is resting on the surface (no touch point).
     */
    virtual bool positionValid() const
    {
        return true;
    }
    virtual bool focusUpdatesBlocked()
    {
        return false;
    }

    inline bool inited() const
    {
        return m_inited;
    }
    inline void setInited(bool set)
    {
        m_inited = set;
    }

private:
    bool setAt(Toplevel* toplevel);
    void updateFocus();
    bool updateDecoration();
    void updateInternalWindow(QWindow* window);

    QWindow* findInternalWindow(const QPoint& pos) const;

    struct {
        QPointer<Toplevel> at;
        QMetaObject::Connection surfaceCreatedConnection;
    } m_at;

    struct {
        QPointer<Toplevel> focus;
        QPointer<Decoration::DecoratedClientImpl> decoration;
        QPointer<QWindow> internalWindow;
    } m_focus;

    bool m_inited = false;
};

}
}
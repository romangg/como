/*
    SPDX-FileCopyrightText: 2006 Lubos Lunak <l.lunak@kde.org>
    SPDX-FileCopyrightText: 2010, 2011, 2017 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef KWIN_EFFECTS_X11_H
#define KWIN_EFFECTS_X11_H

#include "render/effects.h"
#include "xcbutils.h"

#include <memory.h>

namespace KWin::render::backend::x11
{
class EffectsMouseInterceptionX11Filter;

class EffectsHandlerImplX11 : public render::effects_handler_impl
{
    Q_OBJECT
public:
    explicit EffectsHandlerImplX11(render::compositor* compositor, render::scene* scene);
    ~EffectsHandlerImplX11() override;

    void defineCursor(Qt::CursorShape shape) override;

protected:
    bool doGrabKeyboard() override;
    void doUngrabKeyboard() override;

    void doStartMouseInterception(Qt::CursorShape shape) override;
    void doStopMouseInterception() override;

    void doCheckInputWindowStacking() override;

private:
    Xcb::Window m_mouseInterceptionWindow;
    std::unique_ptr<EffectsMouseInterceptionX11Filter> m_x11MouseInterception;
};

}

#endif

/*
    SPDX-FileCopyrightText: 2011 Arthur Arlt <a.arlt@stud.uni-heidelberg.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef KWIN_NON_COMPOSITED_OUTLINE_H
#define KWIN_NON_COMPOSITED_OUTLINE_H

#include "render/outline.h"
#include "xcbutils.h"

namespace KWin::render::backend::x11
{

class NonCompositedOutlineVisual : public outline_visual
{
public:
    NonCompositedOutlineVisual(render::outline* outline);
    ~NonCompositedOutlineVisual() override;
    void show() override;
    void hide() override;

private:
    // TODO: variadic template arguments for adding method arguments
    template<typename T>
    void forEachWindow(T method);
    bool m_initialized;
    Xcb::Window m_topOutline;
    Xcb::Window m_rightOutline;
    Xcb::Window m_bottomOutline;
    Xcb::Window m_leftOutline;
};

template<typename T>
inline void NonCompositedOutlineVisual::forEachWindow(T method)
{
    (m_topOutline.*method)();
    (m_rightOutline.*method)();
    (m_bottomOutline.*method)();
    (m_leftOutline.*method)();
}

}

#endif

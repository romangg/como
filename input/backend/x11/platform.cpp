/*
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "platform.h"

#include "cursor.h"
#include "window_selector.h"

#if HAVE_X11_XINPUT
#include "input/backend/x11/xinput_integration.h"
#endif

#include "input/keyboard_redirect.h"
#include "input/logging.h"
#include "main.h"

#include <QX11Info>

namespace KWin::input::backend::x11
{

platform::platform()
    : input::platform()
{
#if HAVE_X11_XINPUT
    if (!qEnvironmentVariableIsSet("KWIN_NO_XI2")) {
        xinput.reset(new xinput_integration(QX11Info::display()));
        xinput->init();
        if (!xinput->hasXinput()) {
            xinput.reset();
        } else {
            connect(kwinApp(),
                    &Application::startup_finished,
                    xinput.get(),
                    &xinput_integration::startListening);
        }
    }
#endif
}

platform::~platform() = default;

#if HAVE_X11_XINPUT
void create_cursor(platform* platform)
{
    auto const is_xinput_avail = platform->xinput != nullptr;
    auto cursor = new x11::cursor(is_xinput_avail);
    platform->cursor.reset(cursor);

    if (is_xinput_avail) {
        platform->xinput->setCursor(cursor);

        // We know we have xkb already.
        auto xkb = platform->redirect->keyboard()->xkb();
        xkb->setConfig(kwinApp()->kxkbConfig());
        xkb->reconfigure();
    }
}
#else
void create_cursor(platform* platform)
{
    auto cursor = new x11::cursor(false);
    platform->cursor.reset(cursor);
}
#endif

void platform::start_interactive_window_selection(std::function<void(KWin::Toplevel*)> callback,
                                                  QByteArray const& cursorName)
{
    if (!window_sel) {
        window_sel.reset(new window_selector);
    }
    window_sel->start(callback, cursorName);
}

void platform::start_interactive_position_selection(std::function<void(QPoint const&)> callback)
{
    if (!window_sel) {
        window_sel.reset(new window_selector);
    }
    window_sel->start(callback);
}

}
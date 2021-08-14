/*
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "compositor.h"

#include "output.h"
#include "presentation.h"

#include "abstract_wayland_output.h"
#include "platform.h"
#include "scene.h"
#include "wayland_server.h"
#include "win/scene.h"
#include "workspace.h"

namespace KWin::render::wayland
{

compositor* compositor::create(QObject* parent)
{
    assert(!kwinApp()->compositor);
    auto compositor = new wayland::compositor(parent);
    kwinApp()->compositor = compositor;
    return compositor;
}

void compositor::addRepaint(QRegion const& region)
{
    if (!isActive()) {
        return;
    }
    for (auto& [key, output] : outputs) {
        output->add_repaint(region);
    }
}

void compositor::check_idle()
{
    for (auto& [key, output] : outputs) {
        if (!output->idle) {
            return;
        }
    }
    scene()->idle();
}

void compositor::swapped(AbstractWaylandOutput* output)
{
    auto render_output = outputs.at(output).get();
    render_output->swapped_sw();
}

void compositor::swapped(AbstractWaylandOutput* output, unsigned int sec, unsigned int usec)
{
    auto render_output = outputs.at(output).get();
    render_output->swapped_hw(sec, usec);
}

compositor::compositor(QObject* parent)
    : render::compositor(parent)
    , presentation(new render::wayland::presentation(this))
{
    if (!presentation->init_clock(kwinApp()->platform()->supportsClockId(),
                                  kwinApp()->platform()->clockId())) {
        qCCritical(KWIN_CORE) << "Presentation clock failed. Exit.";
        qApp->quit();
    }

    connect(kwinApp(),
            &Application::x11ConnectionAboutToBeDestroyed,
            this,
            &compositor::destroyCompositorSelection);

    for (auto output : kwinApp()->platform()->enabledOutputs()) {
        auto wl_out = static_cast<AbstractWaylandOutput*>(output);
        outputs.emplace(wl_out, new render::wayland::output(wl_out, this));
    }

    connect(kwinApp()->platform(), &Platform::output_added, this, [this](auto output) {
        auto wl_out = static_cast<AbstractWaylandOutput*>(output);
        outputs.emplace(wl_out, new render::wayland::output(wl_out, this));
    });

    connect(kwinApp()->platform(), &Platform::output_removed, this, [this](auto output) {
        for (auto it = outputs.begin(); it != outputs.end(); ++it) {
            if (it->first == output) {
                outputs.erase(it);
                break;
            }
        }
        if (auto workspace = Workspace::self()) {
            for (auto& win : workspace->windows()) {
                remove_all(win->repaint_outputs, output);
            }
        }
    });

    connect(workspace(), &Workspace::destroyed, this, [this] {
        for (auto& [key, output] : outputs) {
            output->delay_timer.stop();
        }
    });

    start();
}

compositor::~compositor() = default;

void compositor::schedule_repaint(Toplevel* window)
{
    if (!isActive()) {
        return;
    }

    if (!kwinApp()->platform()->areOutputsEnabled()) {
        return;
    }

    for (auto& [base, output] : outputs) {
        if (!win::visible_rect(window).intersected(base->geometry()).isEmpty()) {
            output->set_delay_timer();
        }
    }
}

void compositor::toggleCompositing()
{
    // For the shortcut. Not possible on Wayland because we always composite.
}

void compositor::start()
{
    if (!render::compositor::setupStart()) {
        // Internal setup failed, abort.
        return;
    }

    if (Workspace::self()) {
        startupWithWorkspace();
    } else {
        connect(kwinApp(), &Application::workspaceCreated, this, &compositor::startupWithWorkspace);
    }
}

std::deque<Toplevel*> compositor::performCompositing()
{
    for (auto& [output, render_output] : outputs) {
        render_output->run();
    }

    return std::deque<Toplevel*>();
}

}
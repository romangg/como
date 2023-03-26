/*
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "output.h"

#include "platform.h"

#include "base/wayland/output_helpers.h"
#include "render/backend/wlroots/output.h"
#include "render/backend/wlroots/platform.h"
#include "render/wayland/compositor.h"
#include "render/wayland/presentation.h"
#include "utils/gamma_ramp.h"

#include <chrono>
#include <wayland_logging.h>

namespace KWin::base::backend::wlroots
{

using abstract_output_t = base::wayland::output<base::wayland::platform>;

using render_platform = render::backend::wlroots::platform<platform>;
using render_output = render::backend::wlroots::output<output, render_platform>;

template<typename AbstractRenderOutput>
render_output* get_render(std::unique_ptr<AbstractRenderOutput>& output)
{
    return static_cast<render_output*>(output.get());
}

static void handle_destroy(wl_listener* listener, void* /*data*/)
{
    base::event_receiver<output>* event_receiver_struct
        = wl_container_of(listener, event_receiver_struct, event);
    auto output = event_receiver_struct->receiver;

    output->native = nullptr;
    delete output;
}

output::output(wlr_output* wlr_out, wlroots::platform* platform)
    : abstract_output_t(*platform)
    , native{wlr_out}
    , platform{platform}
{
    wlr_out->data = this;

    destroy_rec.receiver = this;
    destroy_rec.event.notify = handle_destroy;
    wl_signal_add(&wlr_out->events.destroy, &destroy_rec.event);

    QVector<Wrapland::Server::output_mode> modes;

    Wrapland::Server::output_mode current_mode;
    if (auto wlr_mode = wlr_out->current_mode) {
        current_mode.size = QSize(wlr_mode->width, wlr_mode->height);
        current_mode.refresh_rate = wlr_mode->refresh;
    }

    auto add_mode = [&modes, &current_mode, &wlr_out](int id, int width, int height, int refresh) {
        Wrapland::Server::output_mode mode;
        mode.id = id;
        mode.size = QSize(width, height);

        if (wlr_out->current_mode && mode.size == current_mode.size
            && refresh == current_mode.refresh_rate) {
            current_mode.id = id;
        }

        // TODO(romangg): We fall back to 60 here as we assume >0 in other code paths, but in
        //                general 0 is a valid value in Wayland protocol which specifies that the
        //                refresh rate is undefined.
        mode.refresh_rate = refresh ? refresh : 60;

        modes.push_back(mode);
    };

    if (wl_list_empty(&wlr_out->modes)) {
        add_mode(0, wlr_out->width, wlr_out->height, wlr_out->refresh);
    } else {
        wlr_output_mode* wlr_mode;
        auto count = 0;
        wl_list_for_each(wlr_mode, &wlr_out->modes, link)
        {
            add_mode(count, wlr_mode->width, wlr_mode->height, wlr_mode->refresh);
            count++;
        }
    }

    auto const make = std::string(wlr_out->make ? wlr_out->make : "");
    auto const model = std::string(wlr_out->model ? wlr_out->model : "");
    auto const serial = std::string(wlr_out->serial ? wlr_out->serial : "");

    init_interfaces(wlr_out->name,
                    make,
                    model,
                    serial,
                    QSize(wlr_out->phys_width, wlr_out->phys_height),
                    modes,
                    current_mode.id != -1 ? &current_mode : nullptr);
    render
        = std::make_unique<render_output>(*this, static_cast<render_platform&>(*platform->render));
}

output::~output()
{
    wl_list_remove(&destroy_rec.event.link);
    if (native) {
        wlr_output_destroy(native);
    }
    if (platform) {
        remove_all(platform->outputs, this);
        remove_all(platform->all_outputs, this);
        platform->server->output_manager->commit_changes();
        Q_EMIT platform->output_removed(this);
    }
}

bool output::disable_native()
{
    wlr_output_enable(native, false);

    if (!wlr_output_test(native)) {
        qCWarning(KWIN_WL) << "Failed test commit on disabling output.";
        // Failed test commit. Switch enabling back.
        wlr_output_enable(native, true);
        return false;
    }

    get_render(render)->disable();
    wlr_output_commit(native);

    return true;
}

void output::update_enablement(bool enable)
{
    if (enable) {
        platform->enable_output(this);
        get_render(render)->reset();
    } else {
        disable_native();
        platform->disable_output(this);
    }
}

void output::update_dpms(base::dpms_mode mode)
{
    auto set_on = mode == base::dpms_mode::on;

    if (set_on) {
        wlr_output_enable(native, true);
        wlr_output_commit(native);
        get_render(render)->reset();
        base::wayland::output_set_dpms_on(*this, *platform);
    } else if (disable_native()) {
        base::wayland::output_set_dmps_off(mode, *this, *platform);
    }
}

void output::update_mode(int mode_index)
{
    // TODO(romangg): Determine target mode more precisly with semantic properties instead of index.
    wlr_output_mode* wlr_mode;
    auto count = 0;

    auto old_mode = native->current_mode;
    wl_list_for_each(wlr_mode, &native->modes, link)
    {
        if (count == mode_index) {
            wlr_output_set_mode(native, wlr_mode);
            if (wlr_output_test(native)) {
                get_render(render)->reset();
            } else {
                qCWarning(KWIN_WL) << "Failed test commit on update mode call.";
                // Set previous mode.
                wlr_output_set_mode(native, old_mode);
            }
            return;
        }
        count++;
    }
}

wl_output_transform to_wl_transform(base::wayland::output_transform tr)
{
    return static_cast<wl_output_transform>(tr);
}

void output::update_transform(base::wayland::output_transform transform)
{
    auto old_transform = native->transform;
    wlr_output_set_transform(native, to_wl_transform(transform));

    if (wlr_output_test(native)) {
        get_render(render)->reset();
    } else {
        qCWarning(KWIN_WL) << "Failed test commit on update transform call.";
        // Set previous transform.
        wlr_output_set_transform(native, old_transform);
    }
}

int output::gamma_ramp_size() const
{
    return wlr_output_get_gamma_size(native);
}

bool output::set_gamma_ramp(gamma_ramp const& gamma)
{
    wlr_output_set_gamma(native, gamma.size(), gamma.red(), gamma.green(), gamma.blue());

    if (wlr_output_test(native)) {
        // Might come early before compositor is created.
        // TODO(romangg): Remove the check once the compositor is guaranteed to exist at this point.
        if (platform->render->compositor) {
            get_render(render)->reset();
        }
        return true;
    } else {
        qCWarning(KWIN_WL) << "Failed test commit on set gamma ramp call.";
        // TODO(romangg): Set previous gamma.
        return false;
    }
}

}

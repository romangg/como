/*
    SPDX-FileCopyrightText: 2020 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "output.h"

#include "compositor.h"
#include "presentation.h"
#include "utils.h"

#include "abstract_wayland_output.h"
#include "effects.h"
#include "platform.h"
#include "wayland_server.h"
#include "win/x11/stacking_tree.h"
#include "workspace.h"

#include "win/transient.h"
#include <kwingltexture.h>

#include "perf/ftrace.h"

#include <Wrapland/Server/surface.h>

namespace KWin::render::wayland
{

static int s_index{0};

output::output(AbstractWaylandOutput* base, wayland::compositor* compositor)
    : index{++s_index}
    , compositor{compositor}
    , base{base}
{
}

void output::add_repaint(QRegion const& region)
{
    auto const capped_region = region.intersected(base->geometry());
    if (capped_region.isEmpty()) {
        return;
    }
    repaints_region += capped_region;
    set_delay_timer();
}

bool output::prepare_repaint(Toplevel* win)
{
    if (!win->has_pending_repaints()) {
        return false;
    }

    auto const repaints = win->repaints();
    if (repaints.intersected(base->geometry()).isEmpty()) {
        // TODO(romangg): Remove win from windows list?
        return false;
    }

    for (auto& [other_base, other_output] : compositor->outputs) {
        if (other_output.get() == this) {
            continue;
        }
        auto const capped_region = repaints.intersected(other_base->geometry());
        if (!capped_region.isEmpty()) {
            other_output->add_repaint(capped_region);
        }
    }

    return true;
}

bool output::prepare_run(QRegion& repaints, std::deque<Toplevel*>& windows)
{
    delay_timer.stop();
    frame_timer.stop();

    // If a buffer swap is still pending, we return to the event loop and
    // continue processing events until the swap has completed.
    if (swap_pending) {
        return false;
    }
    if (!kwinApp()->platform->areOutputsEnabled()) {
        // TODO(romangg): This check is necessary at the moment because of Platform internals
        //                but should go away or be replaced with an output-specific check.
        return false;
    }

    // Create a list of all windows in the stacking order
    windows = workspace()->x_stacking_tree->as_list();
    bool has_window_repaints{false};
    std::deque<Toplevel*> frame_windows;

    for (auto win : windows) {
        if (prepare_repaint(win)) {
            has_window_repaints = true;
        } else if (win->surface()
                   && win->surface()->client() != waylandServer()->xWaylandConnection()
                   && (win->surface()->state().updates & Wrapland::Server::surface_change::frame)
                   && max_coverage_output(win) == base) {
            frame_windows.push_back(win);
        }
        if (win->resetAndFetchDamage()) {
            // Discard the cached lanczos texture
            if (win->transient()->annexed) {
                win = win::lead_of_annexed_transient(win);
            }
            if (win->effectWindow()) {
                auto const texture = win->effectWindow()->data(LanczosCacheRole);
                if (texture.isValid()) {
                    delete static_cast<GLTexture*>(texture.value<void*>());
                    win->effectWindow()->setData(LanczosCacheRole, QVariant());
                }
            }
        }
    }

    // Move elevated windows to the top of the stacking order
    for (auto effect_window : static_cast<EffectsHandlerImpl*>(effects)->elevatedWindows()) {
        auto window = static_cast<EffectWindowImpl*>(effect_window)->window();
        remove_all(windows, window);
        windows.push_back(window);
    }

    if (repaints_region.isEmpty() && !has_window_repaints) {
        idle = true;
        compositor->check_idle();

        // This means the next time we composite it is done without timer delay.
        delay = std::chrono::nanoseconds::zero();

        if (!frame_windows.empty()) {
            // Some windows want a frame event still.
            compositor->presentation->frame(this, frame_windows);
        }
        return false;
    }

    idle = false;

    // Skip windows that are not yet ready for being painted and if screen is locked skip windows
    // that are neither lockscreen nor inputmethod windows.
    //
    // TODO? This cannot be used so carelessly - needs protections against broken clients, the
    // window should not get focus before it's displayed, handle unredirected windows properly and
    // so on.
    for (auto win : windows) {
        if (!win->readyForPainting()) {
            windows.erase(std::remove(windows.begin(), windows.end(), win), windows.end());
        }
        if (waylandServer()->isScreenLocked() && !win->isLockScreen() && !win->isInputMethod()) {
            windows.erase(std::remove(windows.begin(), windows.end(), win), windows.end());
        }
    }

    // Submit pending output repaints and clear the pending field, so that post-pass can add new
    // repaints for the next repaint.
    repaints = repaints_region;
    repaints_region = QRegion();

    return true;
}

std::deque<Toplevel*> output::run()
{
    QRegion repaints;
    std::deque<Toplevel*> windows;

    if (!prepare_run(repaints, windows)) {
        return std::deque<Toplevel*>();
    }

    auto const ftrace_identifier = QString::fromStdString("paint-" + std::to_string(index));

    Perf::Ftrace::begin(ftrace_identifier, ++msc);

    auto now_ns = std::chrono::steady_clock::now().time_since_epoch();
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(now_ns);

    // Start the actual painting process.
    auto const duration = compositor->scene()->paint(base, repaints, windows, now);

    paint_durations.update(std::chrono::nanoseconds(duration));
    retard_next_run();

    if (!windows.empty()) {
        compositor->presentation->lock(this, windows);
    }

    Perf::Ftrace::end(ftrace_identifier, msc);

    return windows;
}

void output::dry_run()
{
    auto windows = workspace()->x_stacking_tree->as_list();
    std::deque<Toplevel*> frame_windows;

    for (auto win : windows) {
        if (!win->surface() || win->surface()->client() == waylandServer()->xWaylandConnection()) {
            continue;
        }
        if (!(win->surface()->state().updates & Wrapland::Server::surface_change::frame)) {
            continue;
        }
        frame_windows.push_back(win);
    }
    compositor->presentation->frame(this, frame_windows);
}

void output::swapped(presentation_data const& data)
{
    compositor->presentation->presented(this, data);

    if (!swap_pending) {
        qCWarning(KWIN_CORE) << "render::wayland::output::swapped called but no swap pending.";
        return;
    }
    swap_pending = false;

    set_delay(data);
    delay_timer.stop();
    set_delay_timer();
}

std::chrono::nanoseconds output::refresh_length() const
{
    return std::chrono::nanoseconds(1000 * 1000 * (1000 * 1000 / base->refreshRate()));
}

void output::set_delay(presentation_data const& data)
{
    auto now = std::chrono::steady_clock::now().time_since_epoch();

    // The gap between the last presentation on the display and us now calculating the delay.
    auto vblank_to_now = now - data.when;

    // The refresh cycle length either from the presentation data, or if not available, our guess.
    auto const refresh
        = data.refresh > std::chrono::nanoseconds::zero() ? data.refresh : refresh_length();

    // Some relative gap to factor in the unknown time the hardware needs to put a rendered image
    // onto the scanout buffer.
    auto const hw_margin = refresh / 10;

    // We try to delay the next paint shortly before next vblank factoring in our margins.
    auto try_delay = refresh - vblank_to_now - hw_margin - paint_durations.get_max();

    // If our previous margins were too large we don't delay. We would likely miss the next vblank.
    delay = std::max(try_delay, std::chrono::nanoseconds::zero());
}

void output::set_delay_timer()
{
    if (delay_timer.isActive() || swap_pending || !base->dpmsOn()) {
        // Abort since we will composite when the timer runs out or the timer will only get
        // started at buffer swap.
        return;
    }

    // In milliseconds.
    auto const wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(delay);

    auto const ftrace_identifier = QString::fromStdString("timer-" + std::to_string(index));
    Perf::Ftrace::mark(ftrace_identifier + QString::number(wait_time.count()));

    // Force 4fps minimum:
    delay_timer.start(std::min(wait_time, std::chrono::milliseconds(250)).count(), this);
}

void output::request_frame(Toplevel* window)
{
    if (swap_pending || delay_timer.isActive() || frame_timer.isActive() || !base->dpmsOn()) {
        // Frame will be received when timer runs out.
        return;
    }

    compositor->presentation->frame(this, {window});
    frame_timer.start(
        std::chrono::duration_cast<std::chrono::milliseconds>(refresh_length()).count(), this);
}

void output::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == delay_timer.timerId()) {
        run();
        return;
    }
    if (event->timerId() == frame_timer.timerId()) {
        dry_run();
        return;
    }
    QObject::timerEvent(event);
}

void output::retard_next_run()
{
    if (compositor->scene()->hasSwapEvent()) {
        // We wait on an explicit callback from the backend to unlock next composition runs.
        return;
    }
    delay = refresh_length();
    set_delay_timer();
}

}

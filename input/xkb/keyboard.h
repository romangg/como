/*
    SPDX-FileCopyrightText: 2021 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "keymap.h"
#include "manager.h"

#include "input/event.h"
#include "input/types.h"

#include <functional>

struct xkb_compose_state;
struct xkb_context;
struct xkb_keymap;
struct xkb_state;

typedef uint32_t xkb_mod_index_t;
typedef uint32_t xkb_led_index_t;
typedef uint32_t xkb_keysym_t;
typedef uint32_t xkb_layout_index_t;

namespace KWin::input::xkb
{

struct modifiers {
    xkb_mod_index_t depressed{0};
    xkb_mod_index_t latched{0};
    xkb_mod_index_t locked{0};
};

class KWIN_EXPORT keyboard : public QObject
{
    Q_OBJECT
public:
    keyboard(xkb::manager& manager);
    ~keyboard() override;

    void install_keymap(int fd, uint32_t size);

    void update_from_default();
    void update(std::shared_ptr<xkb::keymap> keymap, std::vector<std::string> const& layouts);

    void update_modifiers(uint32_t modsDepressed,
                          uint32_t modsLatched,
                          uint32_t modsLocked,
                          uint32_t group);
    void update_key(uint32_t key, key_state state);

    xkb_keysym_t to_keysym(uint32_t key);

    std::string to_string(xkb_keysym_t keysym);
    Qt::Key to_qt_key(xkb_keysym_t keysym,
                      uint32_t scanCode = 0,
                      Qt::KeyboardModifiers modifiers = Qt::KeyboardModifiers(),
                      bool superAsMeta = false) const;

    Qt::KeyboardModifiers modifiers_relevant_for_global_shortcuts(uint32_t scanCode = 0) const;

    bool should_key_repeat(uint32_t key) const;

    void switch_to_next_layout();
    void switch_to_previous_layout();
    bool switch_to_layout(xkb_layout_index_t layout);

    std::string layout_name_from_index(xkb_layout_index_t index) const;
    std::string const& layout_short_name_from_index(int index) const;

    std::string layout_name() const;
    uint32_t layouts_count() const;

    /**
     * Forwards the current modifier state to the windowing system.
     */
    void forward_modifiers();

    xkb_state* state{nullptr};
    std::shared_ptr<xkb::keymap> keymap;
    uint32_t layout{0};
    keyboard_leds leds{keyboard_leds::none};
    xkb_keysym_t keysym{XKB_KEY_NoSymbol};
    std::vector<std::string> layouts;

    Qt::KeyboardModifiers qt_modifiers{Qt::NoModifier};

    /// This XKB keyboard is owned by a third party. Keymap and layouts are not configurable by us.
    bool foreign_owned{false};
    bool startup_num_lock_done{false};

    std::function<void(xkb::keymap*, modifiers const&, uint32_t)> forward_modifiers_impl;

Q_SIGNALS:
    void layout_changed();
    void layouts_changed();
    void leds_changed(keyboard_leds leds);

private:
    void update_keymap(std::shared_ptr<xkb::keymap> keymap);
    void update_modifiers();
    void update_consumed_modifiers(uint32_t key);

    void evaluate_startup_num_lock();

    struct {
        xkb_mod_index_t shift{0};
        xkb_mod_index_t caps{0};
        xkb_mod_index_t ctrl{0};
        xkb_mod_index_t alt{0};
        xkb_mod_index_t meta{0};
        xkb_mod_index_t num{0};
    } modifiers_indices;

    struct {
        xkb_led_index_t num{0};
        xkb_led_index_t caps{0};
        xkb_led_index_t scroll{0};
    } leds_indices;

    modifiers modifier_state;

    Qt::KeyboardModifiers qt_modifiers_consumed{Qt::NoModifier};
    xkb_compose_state* compose_state{nullptr};
    xkb::manager& manager;
};

}

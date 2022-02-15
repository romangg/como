/********************************************************************
KWin - the KDE window manager
This file is part of the KDE project.

Copyright (C) 2012 Martin Gräßlin <mgraesslin@kde.org>

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
#ifndef KWIN_MOCK_TABBOX_HANDLER_H
#define KWIN_MOCK_TABBOX_HANDLER_H

#include "../../win/tabbox/tabboxhandler.h"

namespace KWin
{
class MockTabBoxHandler : public TabBox::TabBoxHandler
{
    Q_OBJECT
public:
    MockTabBoxHandler(QObject* parent = nullptr);
    ~MockTabBoxHandler() override;
    void activate_and_close() override
    {
    }
    std::weak_ptr<TabBox::TabBoxClient> active_client() const override;
    void set_active_client(const std::weak_ptr<TabBox::TabBoxClient>& client);
    int active_screen() const override
    {
        return 0;
    }
    std::weak_ptr<TabBox::TabBoxClient> client_to_add_to_list(TabBox::TabBoxClient* client,
                                                              int desktop) const override;
    int current_desktop() const override
    {
        return 1;
    }
    std::weak_ptr<TabBox::TabBoxClient> desktop_client() const override
    {
        return std::weak_ptr<TabBox::TabBoxClient>();
    }
    QString desktop_name(int desktop) const override
    {
        Q_UNUSED(desktop)
        return "desktop 1";
    }
    QString desktop_name(TabBox::TabBoxClient* client) const override
    {
        Q_UNUSED(client)
        return "desktop";
    }
    void elevate_client(TabBox::TabBoxClient* c, QWindow* tabbox, bool elevate) const override
    {
        Q_UNUSED(c)
        Q_UNUSED(tabbox)
        Q_UNUSED(elevate)
    }
    virtual void hideOutline()
    {
    }
    std::weak_ptr<TabBox::TabBoxClient>
    next_client_focus_chain(TabBox::TabBoxClient* client) const override;
    std::weak_ptr<TabBox::TabBoxClient> first_client_focus_chain() const override;
    bool is_in_focus_chain(TabBox::TabBoxClient* client) const override;
    int next_desktop_focus_chain(int desktop) const override
    {
        Q_UNUSED(desktop)
        return 1;
    }
    int number_of_desktops() const override
    {
        return 1;
    }
    bool is_kwin_compositing() const override
    {
        return false;
    }
    void raise_client(TabBox::TabBoxClient* c) const override
    {
        Q_UNUSED(c)
    }
    void restack(TabBox::TabBoxClient* c, TabBox::TabBoxClient* under) override
    {
        Q_UNUSED(c)
        Q_UNUSED(under)
    }
    virtual void show_outline(const QRect& outline)
    {
        Q_UNUSED(outline)
    }
    TabBox::TabBoxClientList stacking_order() const override
    {
        return TabBox::TabBoxClientList();
    }
    void grabbed_key_event(QKeyEvent* event) const override;

    void highlight_windows(TabBox::TabBoxClient* window = nullptr,
                           QWindow* controller = nullptr) override
    {
        Q_UNUSED(window)
        Q_UNUSED(controller)
    }

    bool no_modifier_grab() const override
    {
        return false;
    }

    // mock methods
    std::weak_ptr<TabBox::TabBoxClient> createMockWindow(const QString& caption);
    void closeWindow(TabBox::TabBoxClient* client);

private:
    std::vector<std::shared_ptr<TabBox::TabBoxClient>> m_windows;
    std::weak_ptr<TabBox::TabBoxClient> m_activeClient;
};
} // namespace KWin
#endif

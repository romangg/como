/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright 2019 Roman Gilg <subdiff@gmail.com>

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

#include "drag.h"

#include <QPoint>

namespace Wrapland::Server
{
class data_source;
}

namespace KWin
{
class Toplevel;

namespace Xwl
{
enum class DragEventReply;
class Xvisit;

using DnDActions = Wrapland::Server::dnd_actions;

class WlToXDrag : public Drag
{
    Q_OBJECT

public:
    explicit WlToXDrag(Dnd* dnd);

    DragEventReply moveFilter(Toplevel* target, QPoint const& pos) override;
    bool handleClientMessage(xcb_client_message_event_t* event) override;

    bool end() override;

    Wrapland::Server::data_source* dataSourceIface() const
    {
        return m_dsi;
    }

private:
    Wrapland::Server::data_source* m_dsi;
    Xvisit* m_visit = nullptr;

    Q_DISABLE_COPY(WlToXDrag)
};

// visit to an X window
class Xvisit : public QObject
{
    Q_OBJECT

public:
    // TODO: handle ask action

    Xvisit(WlToXDrag* drag, Toplevel* target);

    bool handleClientMessage(xcb_client_message_event_t* event);
    bool handleStatus(xcb_client_message_event_t* event);
    bool handleFinished(xcb_client_message_event_t* event);

    void sendPosition(QPointF const& globalPos);
    void leave();

    bool finished() const
    {
        return m_state.finished;
    }
    Toplevel* target() const
    {
        return m_target;
    }

Q_SIGNALS:
    void finish(Xvisit* self);

private:
    void sendEnter();
    void sendDrop(uint32_t time);
    void sendLeave();

    void receiveOffer();
    void enter();

    void retrieveSupportedActions();
    void determineProposedAction();
    void requestDragAndDropAction();
    void setProposedAction();

    void drop();

    void doFinish();
    void stopConnections();

    WlToXDrag* m_drag;
    Toplevel* m_target;
    uint32_t m_version = 0;

    QMetaObject::Connection m_enterConnection;
    QMetaObject::Connection m_motionConnection;
    QMetaObject::Connection m_actionConnection;
    QMetaObject::Connection m_dropConnection;

    struct {
        bool pending = false;
        bool cached = false;
        QPoint cache;
    } m_pos;

    // supported by the Wl source
    DnDActions m_supportedActions = DnDAction::none;
    // preferred by the X client
    DnDAction m_preferredAction = DnDAction::none;
    // decided upon by the compositor
    DnDAction m_proposedAction = DnDAction::none;

    struct {
        bool entered = false;
        bool dropped = false;
        bool finished = false;
    } m_state;

    bool m_accepts = false;

    Q_DISABLE_COPY(Xvisit)
};

}
}

/*
 * The Restful Matching-Engine.
 * Copyright (C) 2013, 2018 Swirly Cloud Limited.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if
 * not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#include "TcpAcceptor.hpp"

namespace swirly {

using namespace std;

TcpAcceptor::TcpAcceptor(Reactor& r, const Endpoint& ep)
  : EventHandler{r}
  , serv_{ep.protocol()}
{
    serv_.setSoReuseAddr(true);
    serv_.bind(ep);
    serv_.listen(SOMAXCONN);
    tok_ = r.subscribe(*serv_, Reactor::In, self());
}

TcpAcceptor::~TcpAcceptor() noexcept = default;

void TcpAcceptor::doReady(int fd, FileEvents events, Time now)
{
    Endpoint ep;
    IoSocket sock{sys::accept(fd, ep), serv_.family()};
    doAccept(move(sock), ep, now);
}

} // namespace swirly

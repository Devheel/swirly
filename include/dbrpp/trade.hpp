/*
 *  Copyright (C) 2013 Mark Aylett <mark.aylett@gmail.com>
 *
 *  This file is part of Doobry written by Mark Aylett.
 *
 *  Doobry is free software; you can redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  Doobry is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 *  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with this program; if
 *  not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301 USA.
 */
#ifndef DBRPP_TRADE_HPP
#define DBRPP_TRADE_HPP

#include <dbrpp/rec.hpp>

namespace dbr {

class TradeRef {
    DbrTrade* impl_;
public:
    explicit
    TradeRef(DbrTrade& impl) noexcept
        : impl_{&impl}
    {
    }
    operator DbrTrade&() const noexcept
    {
        return *impl_;
    }
    DbrTrade*
    c_arg() const noexcept
    {
        return impl_;
    }
    bool
    operator ==(TradeRef rhs) const noexcept
    {
        return impl_->id == rhs.impl_->id;
    }
    bool
    operator !=(TradeRef rhs) const noexcept
    {
        return impl_->id != rhs.impl_->id;
    }
    DbrIden
    id() const noexcept
    {
        return impl_->id;
    }
    DbrIden
    match() const noexcept
    {
        return impl_->match;
    }
    DbrIden
    order() const noexcept
    {
        return impl_->order;
    }
    int
    order_rev() const noexcept
    {
        return impl_->order_rev;
    }
    TraderRecRef
    trec() const noexcept
    {
        return TraderRecRef{*impl_->trader.rec};
    }
    AccntRecRef
    arec() const noexcept
    {
        return AccntRecRef{*impl_->accnt.rec};
    }
    ContrRecRef
    crec() const noexcept
    {
        return ContrRecRef{*impl_->contr.rec};
    }
    DbrDate
    settl_date() const noexcept
    {
        return impl_->settl_date;
    }
    Ref
    ref() const noexcept
    {
        return Ref{impl_->ref};
    }
    AccntRecRef
    cpty() const noexcept
    {
        return AccntRecRef{*impl_->cpty.rec};
    }
    int
    role() const noexcept
    {
        return impl_->role;
    }
    int
    action() const noexcept
    {
        return impl_->action;
    }
    DbrTicks
    ticks() const noexcept
    {
        return impl_->ticks;
    }
    DbrLots
    resd() const noexcept
    {
        return impl_->resd;
    }
    DbrLots
    exec() const noexcept
    {
        return impl_->exec;
    }
    DbrLots
    lots() const noexcept
    {
        return impl_->lots;
    }
    DbrMillis
    created() const noexcept
    {
        return impl_->created;
    }
    DbrMillis
    modified() const noexcept
    {
        return impl_->modified;
    }
};

inline std::ostream&
operator <<(std::ostream& os, TradeRef trade)
{
    return os << "id=" << trade.id()
              << ",match=" << trade.match()
              << ",order=" << trade.order()
              << ",order_rev=" << trade.order_rev()
              << ",trec=" << trade.trec().mnem()
              << ",arec=" << trade.arec().mnem()
              << ",crec=" << trade.crec().mnem()
              << ",settl_date=" << trade.settl_date()
              << ",ref=" << trade.ref()
              << ",cpty=" << trade.cpty().mnem()
              << ",role=" << trade.role()
              << ",action=" << trade.action()
              << ",ticks=" << trade.ticks()
              << ",resd=" << trade.resd()
              << ",exec=" << trade.exec()
              << ",lots=" << trade.lots()
              << ",created=" << trade.created()
              << ",modified=" << trade.modified();
}

inline size_t
trade_len(const DbrTrade& trade, DbrBool enriched) noexcept
{
    return dbr_trade_len(&trade, enriched);
}

inline char*
write_trade(char* buf, const DbrTrade& trade, DbrBool enriched) noexcept
{
    return dbr_write_trade(buf, &trade, enriched);
}

inline const char*
read_trade(const char* buf, DbrTrade& trade)
{
    buf = dbr_read_trade(buf, &trade);
    if (!buf)
        throw_exception();
    return buf;
}
} // dbr

#endif // DBRPP_TRADE_HPP

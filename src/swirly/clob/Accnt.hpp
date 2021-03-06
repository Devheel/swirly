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
#ifndef SWIRLY_CLOB_ACCNT_HPP
#define SWIRLY_CLOB_ACCNT_HPP

#include <swirly/fin/Exception.hpp>
#include <swirly/fin/Exec.hpp>
#include <swirly/fin/MarketId.hpp>
#include <swirly/fin/Order.hpp>
#include <swirly/fin/Posn.hpp>

#include <swirly/util/Set.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <boost/circular_buffer.hpp>
#pragma GCC diagnostic pop

namespace swirly {

class Accnt;

using AccntPtr = std::unique_ptr<Accnt>;
using ConstAccntPtr = std::unique_ptr<const Accnt>;

class SWIRLY_API Accnt : public Comparable<Accnt> {
  public:
    Accnt(Symbol symbol, std::size_t maxExecs) noexcept
      : symbol_{symbol}
      , execs_{maxExecs}
    {
    }
    ~Accnt() noexcept;

    // Copy.
    Accnt(const Accnt&) = delete;
    Accnt& operator=(const Accnt&) = delete;

    // Move.
    Accnt(Accnt&&);
    Accnt& operator=(Accnt&&) = delete;

    template <typename... ArgsT>
    static AccntPtr make(ArgsT&&... args)
    {
        return std::make_unique<Accnt>(std::forward<ArgsT>(args)...);
    }

    int compare(const Accnt& rhs) const noexcept { return symbol_.compare(rhs.symbol_); }
    bool exists(std::string_view ref) const noexcept { return refIdx_.find(ref) != refIdx_.end(); }
    auto symbol() const noexcept { return symbol_; }
    const auto& orders() const noexcept { return orders_; }
    const auto& execs() const noexcept { return execs_; }
    const auto& trades() const noexcept { return trades_; }
    const Exec& trade(Id64 marketId, Id64 id) const
    {
        auto it = trades_.find(marketId, id);
        if (it == trades_.end()) {
            throw NotFoundException{errMsg() << "trade '" << id << "' does not exist"};
        }
        return *it;
    }
    const auto& posns() const noexcept { return posns_; }

    auto& orders() noexcept { return orders_; }
    Order& order(Id64 marketId, Id64 id)
    {
        auto it = orders_.find(marketId, id);
        if (it == orders_.end()) {
            throw OrderNotFoundException{errMsg() << "order '" << id << "' does not exist"};
        }
        return *it;
    }
    Order& order(std::string_view ref)
    {
        auto it = refIdx_.find(ref);
        if (it == refIdx_.end()) {
            throw OrderNotFoundException{errMsg() << "order '" << ref << "' does not exist"};
        }
        return *it;
    }
    void insertOrder(const OrderPtr& order) noexcept
    {
        assert(order->accnt() == symbol_);
        orders_.insert(order);
        if (!order->ref().empty()) {
            refIdx_.insert(order);
        }
    }
    OrderPtr removeOrder(const Order& order) noexcept
    {
        assert(order.accnt() == symbol_);
        if (!order.ref().empty()) {
            refIdx_.remove(order);
        }
        return orders_.remove(order);
    }
    void pushExecBack(const ConstExecPtr& exec) noexcept
    {
        assert(exec->accnt() == symbol_);
        execs_.push_back(exec);
    }
    void pushExecFront(const ConstExecPtr& exec) noexcept
    {
        assert(exec->accnt() == symbol_);
        execs_.push_front(exec);
    }
    void insertTrade(const ExecPtr& trade) noexcept
    {
        assert(trade->accnt() == symbol_);
        assert(trade->state() == State::Trade);
        trades_.insert(trade);
    }
    ConstExecPtr removeTrade(const Exec& trade) noexcept
    {
        assert(trade.accnt() == symbol_);
        return trades_.remove(trade);
    }
    /**
     * Throws std::bad_alloc.
     */
    PosnPtr posn(Id64 marketId, Symbol instr, JDay settlDay);

    void insertPosn(const PosnPtr& posn) noexcept
    {
        assert(posn->accnt() == symbol_);
        posns_.insert(posn);
    }
    boost::intrusive::set_member_hook<> symbolHook_;
    using PosnSet = IdSet<Posn, MarketIdTraits<Posn>>;

  private:
    const Symbol symbol_;
    OrderIdSet orders_;
    boost::circular_buffer<ConstExecPtr> execs_;
    ExecIdSet trades_;
    PosnSet posns_;
    OrderRefSet refIdx_;
};

using AccntSet = SymbolSet<Accnt>;

} // namespace swirly

#endif // SWIRLY_CLOB_ACCNT_HPP

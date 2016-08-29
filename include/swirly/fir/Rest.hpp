/*
 * The Restful Matching-Engine.
 * Copyright (C) 2013, 2016 Swirly Cloud Limited.
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
#ifndef SWIRLY_FIR_REST_HPP
#define SWIRLY_FIR_REST_HPP

#include <swirly/fir/EntitySet.hpp>

#include <swirly/fig/Serv.hpp>

namespace swirly {

class SWIRLY_API Rest {
 public:
  Rest(Journ& journ, std::size_t pipeCapacity, std::size_t maxExecs)
    : serv_{journ, pipeCapacity, maxExecs}
  {
  }
  ~Rest() noexcept;

  // Copy.
  Rest(const Rest&) = delete;
  Rest& operator=(const Rest&) = delete;

  // Move.
  Rest(Rest&&);
  Rest& operator=(Rest&&);

  void load(const Model& model, Millis now) { serv_.load(model, now); }

  void getRec(EntitySet es, Millis now, std::ostream& out) const;

  void getAsset(Millis now, std::ostream& out) const;

  void getAsset(Mnem mnem, Millis now, std::ostream& out) const;

  void getContr(Millis now, std::ostream& out) const;

  void getContr(Mnem mnem, Millis now, std::ostream& out) const;

  void getMarket(Millis now, std::ostream& out) const;

  void getMarket(Mnem mnem, Millis now, std::ostream& out) const;

  void getAccnt(Mnem mnem, EntitySet es, std::size_t offset, std::optional<std::size_t> limit,
                Millis now, std::ostream& out) const;

  void getOrder(Mnem accMnem, Millis now, std::ostream& out) const;

  void getOrder(Mnem accMnem, Mnem market, Millis now, std::ostream& out) const;

  void getOrder(Mnem accMnem, Mnem market, Id64 id, Millis now, std::ostream& out) const;

  void getExec(Mnem accMnem, std::size_t offset, std::optional<std::size_t> limit, Millis now,
               std::ostream& out) const;

  void getTrade(Mnem accMnem, Millis now, std::ostream& out) const;

  void getTrade(Mnem accMnem, Mnem market, Millis now, std::ostream& out) const;

  void getTrade(Mnem accMnem, Mnem market, Id64 id, Millis now, std::ostream& out) const;

  void getPosn(Mnem accMnem, Millis now, std::ostream& out) const;

  void getPosn(Mnem accMnem, Mnem contr, Millis now, std::ostream& out) const;

  void getPosn(Mnem accMnem, Mnem contr, IsoDate settlDate, Millis now, std::ostream& out) const;

  void getView(Millis now, std::ostream& out) const;

  void getView(ArrayView<Mnem> markets, Millis now, std::ostream& out) const;

  void postMarket(Mnem mnem, std::string_view display, Mnem contr, IsoDate settlDate,
                  IsoDate expiryDate, MarketState state, Millis now, std::ostream& out);

  void putMarket(Mnem mnem, std::optional<std::string_view> display,
                 std::optional<MarketState> state, Millis now, std::ostream& out);

  void postOrder(Mnem accMnem, Mnem market, std::string_view ref, Side side, Lots lots, Ticks ticks,
                 Lots minLots, Millis now, std::ostream& out);

  void putOrder(Mnem accMnem, Mnem market, ArrayView<Id64> ids, Lots lots, Millis now,
                std::ostream& out);

  void postTrade(Mnem accMnem, Mnem market, std::string_view ref, Side side, Lots lots, Ticks ticks,
                 LiqInd liqInd, Mnem cpty, Millis now, std::ostream& out);

  void deleteTrade(Mnem accMnem, Mnem market, ArrayView<Id64> ids, Millis now);

 private:
  void getOrder(const Accnt& accnt, Millis now, std::ostream& out) const;

  void getExec(const Accnt& accnt, std::size_t offset, std::optional<std::size_t> limit, Millis now,
               std::ostream& out) const;

  void getTrade(const Accnt& accnt, Millis now, std::ostream& out) const;

  void getPosn(const Accnt& accnt, Millis now, std::ostream& out) const;

  Serv serv_;
};

} // swirly

#endif // SWIRLY_FIR_REST_HPP

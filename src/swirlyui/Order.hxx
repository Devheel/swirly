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
#ifndef SWIRLYUI_ORDER_HXX
#define SWIRLYUI_ORDER_HXX

#include "Instr.hxx"

namespace swirly {
namespace ui {
namespace order {

enum class Column : int { //
    CheckState, //
    Accnt, //
    MarketId, //
    Instr, //
    SettlDate, //
    Id, //
    Ref, //
    State, //
    Side, //
    Lots, //
    Price, //
    ResdLots, //
    ExecLots, //
    AvgPrice, //
    LastLots, //
    LastPrice, //
    MinLots, //
    Created, //
    Modified
};
constexpr int ColumnCount{unbox(Column::Modified) + 1};

} // namespace order

class Order {
  public:
    Order(const QString& accnt, Id64 marketId, const Instr& instr, QDate settlDate, Id64 id,
          const QString& ref, State state, Side side, Lots lots, Ticks ticks, Lots resdLots,
          Lots execLots, Cost execCost, Lots lastLots, Ticks lastTicks, Lots minLots,
          const QDateTime& created, const QDateTime& modified);
    Order() = default;
    ~Order() noexcept = default;

    static Order fromJson(const Instr& instr, const QJsonObject& obj);

    OrderKey key() const noexcept { return {marketId_, id_}; }
    const QString& accnt() const noexcept { return accnt_; }
    Id64 marketId() const noexcept { return marketId_; }
    const Instr& instr() const noexcept { return instr_; }
    QDate settlDate() const noexcept { return settlDate_; }
    Id64 id() const noexcept { return id_; }
    const QString& ref() const noexcept { return ref_; }
    State state() const noexcept { return state_; }
    Side side() const noexcept { return side_; }
    Lots lots() const noexcept { return lots_; }
    Ticks ticks() const noexcept { return ticks_; }
    Lots resdLots() const noexcept { return resdLots_; }
    Lots execLots() const noexcept { return execLots_; }
    Cost execCost() const noexcept { return execCost_; }
    Lots lastLots() const noexcept { return lastLots_; }
    Ticks lastTicks() const noexcept { return lastTicks_; }
    Lots minLots() const noexcept { return minLots_; }
    bool done() const noexcept { return resdLots_ == 0_lts; }
    const QDateTime& created() const noexcept { return created_; }
    const QDateTime& modified() const noexcept { return modified_; }

  private:
    QString accnt_{};
    Id64 marketId_{};
    Instr instr_{};
    QDate settlDate_{};
    Id64 id_{};
    QString ref_{};
    State state_{};
    Side side_{};
    Lots lots_{};
    Ticks ticks_{};
    Lots resdLots_{};
    Lots execLots_{};
    Cost execCost_{};
    Lots lastLots_{};
    Ticks lastTicks_{};
    Lots minLots_{};
    QDateTime created_{};
    QDateTime modified_{};
};

QDebug operator<<(QDebug debug, const Order& order);

inline bool isModified(const Order& prev, const Order& next) noexcept
{
    return prev.state() != next.state() //
        || prev.lots() != next.lots() //
        || prev.resdLots() != next.resdLots() //
        || prev.execLots() != next.execLots() //
        || prev.execCost() != next.execCost() //
        || prev.lastLots() != next.lastLots() //
        || prev.lastTicks() != next.lastTicks() //
        || prev.modified() != next.modified();
}

} // namespace ui
} // namespace swirly

Q_DECLARE_METATYPE(swirly::ui::Order)

#endif // SWIRLYUI_ORDER_HXX

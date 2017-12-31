/*
 * The Restful Matching-Engine.
 * Copyright (C) 2013, 2017 Swirly Cloud Limited.
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
#ifndef SWIRLYUI_LEVEL_HXX
#define SWIRLYUI_LEVEL_HXX

#include "Types.hxx"

#include <QMetaType>

class QJsonObject;

namespace swirly {
namespace ui {

class Level {
  public:
    Level(Ticks ticks, Lots lots, int count)
      : ticks_{ticks}
      , lots_{lots}
      , count_{count}
    {
    }
    Level() = default;
    ~Level() noexcept = default;

    static Level fromJson(const QJsonObject& obj);

    Ticks ticks() const noexcept { return ticks_; }
    Lots lots() const noexcept { return lots_; }
    int count() const noexcept { return count_; }

  private:
    Ticks ticks_{};
    Lots lots_{};
    int count_{};
};

QDebug operator<<(QDebug debug, const Level& level);

} // namespace ui
} // namespace swirly

Q_DECLARE_METATYPE(swirly::ui::Level)

#endif // SWIRLYUI_LEVEL_HXX

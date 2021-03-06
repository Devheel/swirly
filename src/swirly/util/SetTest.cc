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
#include "Set.hpp"

#include <swirly/util/String.hpp>

#include <swirly/unit/Test.hpp>

using namespace std;
using namespace swirly;

namespace {

class Foo : public RefCount<Foo, ThreadUnsafePolicy> {
  public:
    Foo(Symbol symbol, string_view display, int& alive) noexcept
      : symbol_{symbol}
      , display_{display}
      , alive_{alive}
    {
        ++alive;
    }
    ~Foo() noexcept { --alive_; }

    auto symbol() const noexcept { return symbol_; }
    auto display() const noexcept { return +display_; }
    boost::intrusive::set_member_hook<> symbolHook_;

  private:
    const Symbol symbol_;
    String<64> display_;
    int& alive_;
};

class Bar : public RefCount<Bar, ThreadUnsafePolicy> {
  public:
    Bar(Id64 id, string_view display, int& alive) noexcept
      : id_{id}
      , display_{display}
      , alive_{alive}
    {
        ++alive;
    }
    ~Bar() noexcept { --alive_; }

    auto id() const noexcept { return id_; }
    auto display() const noexcept { return +display_; }
    boost::intrusive::set_member_hook<> idHook_;

  private:
    const Id64 id_;
    String<64> display_;
    int& alive_;
};

} // namespace

SWIRLY_TEST_CASE(SymbolSet)
{
    int alive{0};
    {
        SymbolSet<Foo> s;

        Foo& foo1{*s.emplace("FOO"sv, "Foo One"sv, alive)};
        SWIRLY_CHECK(alive == 1);
        SWIRLY_CHECK(foo1.symbol() == "FOO"sv);
        SWIRLY_CHECK(foo1.display() == "Foo One"sv);
        SWIRLY_CHECK(s.find("FOO"sv) != s.end());

        // Duplicate.
        Foo& foo2{*s.emplace("FOO"sv, "Foo Two"sv, alive)};
        SWIRLY_CHECK(alive == 1);
        SWIRLY_CHECK(&foo2 == &foo1);

        // Replace.
        Foo& foo3{*s.emplaceOrReplace("FOO"sv, "Foo Three"sv, alive)};
        SWIRLY_CHECK(alive == 1);
        SWIRLY_CHECK(&foo3 != &foo1);
        SWIRLY_CHECK(foo3.symbol() == "FOO"sv);
        SWIRLY_CHECK(foo3.display() == "Foo Three"sv);
    }
    SWIRLY_CHECK(alive == 0);
}

SWIRLY_TEST_CASE(IdSet)
{
    int alive{0};
    {
        IdSet<Bar> s;

        Bar& bar1{*s.emplace(1_id64, "Bar One"sv, alive)};
        SWIRLY_CHECK(alive == 1);
        SWIRLY_CHECK(bar1.id() == 1_id64);
        SWIRLY_CHECK(bar1.display() == "Bar One"sv);
        SWIRLY_CHECK(s.find(1_id64) != s.end());

        // Duplicate.
        Bar& bar2{*s.emplace(1_id64, "Bar Two"sv, alive)};
        SWIRLY_CHECK(alive == 1);
        SWIRLY_CHECK(&bar2 == &bar1);

        // Replace.
        Bar& bar3{*s.emplaceOrReplace(1_id64, "Bar Three"sv, alive)};
        SWIRLY_CHECK(alive == 1);
        SWIRLY_CHECK(&bar3 != &bar1);
        SWIRLY_CHECK(bar3.id() == 1_id64);
        SWIRLY_CHECK(bar3.display() == "Bar Three"sv);
    }
    SWIRLY_CHECK(alive == 0);
}

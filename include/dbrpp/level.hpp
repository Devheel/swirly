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
#ifndef DBRPP_LEVEL_HPP
#define DBRPP_LEVEL_HPP

#include <dbr/types.h>

#include <iostream>

namespace dbr {

class Level {
    DbrLevel impl_;
public:
    explicit
    Level(DbrLevel impl) noexcept
        : impl_{impl}
    {
    }
    explicit
    operator DbrLevel() const noexcept
    {
        return impl_;
    }
    size_t
    count() const noexcept
    {
        return impl_.count;
    }
    DbrTicks
    ticks() const noexcept
    {
        return impl_.ticks;
    }
    // Must be greater than zero.
    DbrLots
    resd() const noexcept
    {
        return impl_.resd;
    }
};

inline std::ostream&
operator <<(std::ostream& os, Level level)
{
    return os << "count=" << level.count()
              << ",ticks=" << level.ticks()
              << ",resd=" << level.resd();
}
} // dbr

#endif // DBRPP_LEVEL_HPP

/*
 *  Copyright (C) 2013, 2014 Mark Aylett <mark.aylett@gmail.com>
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
#include "test.hpp"

#include <dbr/elm/format.h>
#include <dbr/elm/types.h>

#include <limits.h>

TEST_CASE(format_action)
{
    char buf[256];

    *dbr_format(buf, "<%A>", DBR_ACTION_BUY) = '\0';
    check(strcmp(buf, "<BUY>") == 0);

    *dbr_format(buf, "<%A>", DBR_ACTION_SELL) = '\0';
    check(strcmp(buf, "<SELL>") == 0);
}

TEST_CASE(format_role)
{
    char buf[256];

    *dbr_format(buf, "<%R>", DBR_ROLE_MAKER) = '\0';
    check(strcmp(buf, "<MAKER>") == 0);

    *dbr_format(buf, "<%R>", DBR_ROLE_TAKER) = '\0';
    check(strcmp(buf, "<TAKER>") == 0);
}

TEST_CASE(format_state)
{
    char buf[256];

    *dbr_format(buf, "<%S>", DBR_STATE_NEW) = '\0';
    check(strcmp(buf, "<NEW>") == 0);

    *dbr_format(buf, "<%S>", DBR_STATE_REVISE) = '\0';
    check(strcmp(buf, "<REVISE>") == 0);

    *dbr_format(buf, "<%S>", DBR_STATE_CANCEL) = '\0';
    check(strcmp(buf, "<CANCEL>") == 0);

    *dbr_format(buf, "<%S>", DBR_STATE_TRADE) = '\0';
    check(strcmp(buf, "<TRADE>") == 0);
}

TEST_CASE(format_int)
{
    char buf[256];

    *dbr_format(buf, "<%d>", 0) = '\0';
    check(strcmp(buf, "<0>") == 0);

    *dbr_format(buf, "<%d>", -1) = '\0';
    check(strcmp(buf, "<-1>") == 0);

    *dbr_format(buf, "<%i>", INT_MAX) = '\0';
    check(strcmp(buf, "<2147483647>") == 0);

    *dbr_format(buf, "<%i>", INT_MIN) = '\0';
    check(strcmp(buf, "<-2147483648>") == 0);
}

TEST_CASE(format_long)
{
    char buf[256];

    *dbr_format(buf, "<%l>", 0L) = '\0';
    check(strcmp(buf, "<0>") == 0);

    *dbr_format(buf, "<%l>", -1L) = '\0';
    check(strcmp(buf, "<-1>") == 0);

    *dbr_format(buf, "<%l>", LONG_MAX) = '\0';
    check(strcmp(buf, "<9223372036854775807>") == 0);

    *dbr_format(buf, "<%l>", LONG_MIN) = '\0';
    check(strcmp(buf, "<-9223372036854775808>") == 0);
}

TEST_CASE(format_escape)
{
    char buf[256];

    *dbr_format(buf, "%%") = '\0';
    check(strcmp(buf, "%") == 0);
}
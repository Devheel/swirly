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
#ifndef DBR_JOURN_H
#define DBR_JOURN_H

#include <dbr/types.h>

/**
 * @addtogroup Journ
 * @{
 */

struct DbrJournVtbl {
    DbrIden
    (*alloc_id)(DbrJourn journ);

    DbrBool
    (*begin_trans)(DbrJourn journ);

    DbrBool
    (*commit_trans)(DbrJourn journ);

    DbrBool
    (*rollback_trans)(DbrJourn journ);

    DbrBool
    (*insert_order)(DbrJourn journ, struct DbrOrder* order);

    DbrBool
    (*update_order)(DbrJourn journ, DbrIden id, int rev, int status, DbrLots resd, DbrLots exec,
                    DbrLots lots, DbrMillis now);

    DbrBool
    (*archive_order)(DbrJourn journ, DbrIden id, DbrMillis now);

    DbrBool
    (*insert_trade)(DbrJourn journ, struct DbrTrade* trade);

    DbrBool
    (*archive_trade)(DbrJourn journ, DbrIden id, DbrMillis now);
};

static inline DbrIden
dbr_journ_alloc_id(DbrJourn journ)
{
    return journ->vtbl->alloc_id(journ);
}

static inline DbrBool
dbr_journ_begin_trans(DbrJourn journ)
{
    return journ->vtbl->begin_trans(journ);
}

static inline DbrBool
dbr_journ_commit_trans(DbrJourn journ)
{
    return journ->vtbl->commit_trans(journ);
}

static inline DbrBool
dbr_journ_rollback_trans(DbrJourn journ)
{
    return journ->vtbl->rollback_trans(journ);
}

static inline DbrBool
dbr_journ_insert_order(DbrJourn journ, struct DbrOrder* order)
{
    return journ->vtbl->insert_order(journ, order);
}

static inline DbrBool
dbr_journ_update_order(DbrJourn journ, DbrIden id, int rev, int status, DbrLots resd, DbrLots exec,
                       DbrLots lots, DbrMillis now)
{
    return journ->vtbl->update_order(journ, id, rev, status, resd, exec, lots, now);
}

static inline DbrBool
dbr_journ_archive_order(DbrJourn journ, DbrIden id, DbrMillis now)
{
    return journ->vtbl->archive_order(journ, id, now);
}

static inline DbrBool
dbr_journ_insert_trade(DbrJourn journ, struct DbrTrade* trade)
{
    return journ->vtbl->insert_trade(journ, trade);
}

static inline DbrBool
dbr_journ_archive_trade(DbrJourn journ, DbrIden id, DbrMillis now)
{
    return journ->vtbl->archive_trade(journ, id, now);
}

/** @} */

#endif // DBR_JOURN_H
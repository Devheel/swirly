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
#include "sqlite.h"

#include "sqlite3.h"

#include <dbr/conv.h>
#include <dbr/err.h>
#include <dbr/log.h>
#include <dbr/pool.h>
#include <dbr/queue.h>

#include <string.h>

#define INSERT_EXEC_SQL                                                 \
    "INSERT INTO exec (id, order_, trader, accnt, contr, settl_date,"   \
    " ref, state, action, ticks, lots, resd, exec, last_ticks,"         \
    " last_lots, min_lots, match, acked, role, cpty, created,"          \
    " modified)"                                                        \
    " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0,"    \
    " ?, ?, ?, ?)"

#define UPDATE_EXEC_SQL                                                 \
    "UPDATE exec SET acked = 1, modified = ?"                           \
    " WHERE id = ?"

#define SELECT_TRADER_SQL                                               \
    "SELECT id, mnem, display, email"                                   \
    " FROM trader_v ORDER BY id"

#define SELECT_ACCNT_SQL                                                \
    "SELECT id, mnem, display, email"                                   \
    " FROM accnt_v ORDER BY id"

#define SELECT_CONTR_SQL                                                \
    "SELECT id, mnem, display, asset_type, asset, ccy, tick_numer,"     \
    " tick_denom, lot_numer, lot_denom, pip_dp, min_lots, max_lots"     \
    " FROM contr_v ORDER BY id"

#define SELECT_ORDER_SQL                                                \
    "SELECT id, trader, accnt, contr, settl_date, ref, state,"          \
    " action, ticks, lots, resd, exec, last_ticks, last_lots,"          \
    " min_lots, created, modified"                                      \
    " FROM order_ WHERE resd > 0 ORDER BY id"

#define SELECT_TRADE_SQL                                                \
    "SELECT id, order_, trader, accnt, contr, settl_date, ref,"         \
    " action, ticks, lots, resd, exec, last_ticks, last_lots,"          \
    " min_lots, match, role, cpty, created"                             \
    " FROM trade_v WHERE acked = 0 ORDER BY id"

#define SELECT_MEMB_SQL                                                 \
    "SELECT accnt, trader"                                              \
    " FROM memb ORDER BY accnt"

#define SELECT_POSN_SQL                                                 \
    "SELECT accnt, contr, settl_date, action, licks, lots"              \
    " FROM posn_v ORDER BY accnt, contr, settl_date, action"

// Only called if failure occurs during cache load, so no need to free state members as they will
// not have been allocated.

static sqlite3_stmt*
prepare(sqlite3* db, const char* sql)
{
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare(db, sql, -1, &stmt, NULL);
    if (dbr_unlikely(rc != SQLITE_OK)) {
        dbr_err_set(DBR_EIO, sqlite3_errmsg(db));
        stmt = NULL;
    }
    return stmt;
}

static DbrBool
exec_sql(sqlite3* db, const char* sql)
{
    sqlite3_stmt* stmt = prepare(db, sql);
    if (!stmt)
        goto fail1;

    // SQLITE_ROW  100
    // SQLITE_DONE 101

    if (dbr_unlikely(sqlite3_step(stmt) < SQLITE_ROW)) {
        dbr_err_set(DBR_EIO, sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        goto fail1;
    }

    sqlite3_finalize(stmt);
    return DBR_TRUE;
 fail1:
    return DBR_FALSE;
}

static DbrBool
exec_stmt(sqlite3* db, sqlite3_stmt* stmt)
{
    int rc = sqlite3_step(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    if (dbr_unlikely(rc != SQLITE_DONE)) {
        dbr_err_set(DBR_EIO, sqlite3_errmsg(db));
        return DBR_FALSE;
    }
    return DBR_TRUE;
}

#if DBR_DEBUG_LEVEL >= 2
static void
trace_sql(void* unused, const char* sql)
{
    dbr_log_debug2("%s", sql);
}
#endif // DBR_DEBUG_LEVEL >= 2

static inline int
bind_text(sqlite3_stmt* stmt, int col, const char* text, size_t maxlen)
{
    return sqlite3_bind_text(stmt, col, text, strnlen(text, maxlen), SQLITE_STATIC);
}

static DbrBool
bind_insert_exec(struct FirSqlite* sqlite, const struct DbrExec* exec, DbrBool enriched)
{
    enum {
        ID = 1,
        ORDER,
        TRADER,
        ACCNT,
        CONTR,
        SETTL_DATE,
        REF,
        STATE,
        ACTION,
        TICKS,
        LOTS,
        RESD,
        EXEC,
        LAST_TICKS,
        LAST_LOTS,
        MIN_LOTS,
        MATCH,
        ROLE,
        CPTY,
        CREATED,
        MODIFIED
    };
    sqlite3_stmt* stmt = sqlite->insert_exec;
    int rc = sqlite3_bind_int64(stmt, ID, exec->id);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, ORDER, exec->order);
    if (rc != SQLITE_OK)
        goto fail1;

    if (enriched) {

        rc = sqlite3_bind_int64(stmt, TRADER, exec->c.trader.rec->id);
        if (rc != SQLITE_OK)
            goto fail1;

        rc = sqlite3_bind_int64(stmt, ACCNT, exec->c.accnt.rec->id);
        if (rc != SQLITE_OK)
            goto fail1;

        rc = sqlite3_bind_int64(stmt, CONTR, exec->c.contr.rec->id);
        if (rc != SQLITE_OK)
            goto fail1;

    } else {

        rc = sqlite3_bind_int64(stmt, TRADER, exec->c.trader.id_only);
        if (rc != SQLITE_OK)
            goto fail1;

        rc = sqlite3_bind_int64(stmt, ACCNT, exec->c.accnt.id_only);
        if (rc != SQLITE_OK)
            goto fail1;

        rc = sqlite3_bind_int64(stmt, CONTR, exec->c.contr.id_only);
        if (rc != SQLITE_OK)
            goto fail1;
    }

    rc = sqlite3_bind_int(stmt, SETTL_DATE, exec->c.settl_date);
    if (rc != SQLITE_OK)
        goto fail1;

    if (exec->c.ref[0] != '\0')
        rc = bind_text(stmt, REF, exec->c.ref, DBR_REF_MAX);
    else
        rc = sqlite3_bind_null(stmt, REF);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int(stmt, STATE, exec->c.state);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int(stmt, ACTION, exec->c.action);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, TICKS, exec->c.ticks);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, LOTS, exec->c.lots);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, RESD, exec->c.resd);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, EXEC, exec->c.exec);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, LAST_TICKS, exec->c.last_ticks);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, LAST_LOTS, exec->c.last_lots);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, MIN_LOTS, exec->c.min_lots);
    if (rc != SQLITE_OK)
        goto fail1;

    const DbrIden match = exec->match;
    rc = match != 0 ? sqlite3_bind_int64(stmt, MATCH, match) : sqlite3_bind_null(stmt, MATCH);
    if (rc != SQLITE_OK)
        goto fail1;

    const int role = exec->role;
    rc = role != 0 ? sqlite3_bind_int(stmt, ROLE, role) : sqlite3_bind_null(stmt, ROLE);
    if (rc != SQLITE_OK)
        goto fail1;

    if (enriched) {
        if (exec->cpty.rec)
            rc = sqlite3_bind_int64(stmt, CPTY, exec->cpty.rec->id);
        else
            rc = sqlite3_bind_null(stmt, CPTY);
    } else {
        const DbrIden cpty = exec->cpty.id_only;
        rc = cpty != 0 ? sqlite3_bind_int64(stmt, CPTY, cpty) : sqlite3_bind_null(stmt, CPTY);
    }
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, CREATED, exec->created);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, MODIFIED, exec->created);
    if (rc != SQLITE_OK)
        goto fail1;

    return DBR_TRUE;
 fail1:
    dbr_err_set(DBR_EIO, sqlite3_errmsg(sqlite->db));
    sqlite3_clear_bindings(stmt);
    return DBR_FALSE;
}

static DbrBool
bind_update_exec(struct FirSqlite* sqlite, DbrIden id, DbrMillis modified)
{
    enum {
        MODIFIED = 1,
        ID
    };
    sqlite3_stmt* stmt = sqlite->update_exec;
    int rc = sqlite3_bind_int64(stmt, MODIFIED, modified);
    if (rc != SQLITE_OK)
        goto fail1;

    rc = sqlite3_bind_int64(stmt, ID, id);
    if (rc != SQLITE_OK)
        goto fail1;

    return DBR_TRUE;
 fail1:
    dbr_err_set(DBR_EIO, sqlite3_errmsg(sqlite->db));
    sqlite3_clear_bindings(stmt);
    return DBR_FALSE;
}

static ssize_t
select_trader(struct FirSqlite* sqlite, DbrPool pool, struct DbrSlNode** first)
{
    enum {
        ID,
        MNEM,
        DISPLAY,
        EMAIL
    };

    sqlite3_stmt* stmt = prepare(sqlite->db, SELECT_TRADER_SQL);
    if (!stmt)
        goto fail1;

    struct DbrQueue rq;
    dbr_queue_init(&rq);

    size_t size = 0;
    for (;;) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {

            struct DbrRec* rec = dbr_pool_alloc_rec(pool);
            if (!rec)
                goto fail2;
            dbr_rec_init(rec);

            // Header.

            rec->type = DBR_ENTITY_TRADER;
            rec->id = sqlite3_column_int64(stmt, ID);
            strncpy(rec->mnem,
                    (const char*)sqlite3_column_text(stmt, MNEM), DBR_MNEM_MAX);
            strncpy(rec->display,
                    (const char*)sqlite3_column_text(stmt, DISPLAY), DBR_DISPLAY_MAX);

            // Body.

            strncpy(rec->trader.email,
                    (const char*)sqlite3_column_text(stmt, EMAIL), DBR_EMAIL_MAX);
            rec->trader.state = NULL;

            dbr_log_debug3("trader: id=%ld,mnem=%.16s,display=%.64s,email=%.64s",
                           rec->id, rec->mnem, rec->display, rec->trader.email);

            dbr_queue_insert_back(&rq, &rec->shared_node_);
            ++size;

        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            dbr_err_set(DBR_EIO, sqlite3_errmsg(sqlite->db));
            goto fail2;
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    *first = dbr_queue_first(&rq);
    return size;
 fail2:
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    dbr_pool_free_entity_list(pool, DBR_ENTITY_TRADER, dbr_queue_first(&rq));
    *first = NULL;
 fail1:
    return -1;
}

static ssize_t
select_accnt(struct FirSqlite* sqlite, DbrPool pool, struct DbrSlNode** first)
{
    enum {
        ID,
        MNEM,
        DISPLAY,
        EMAIL
    };

    sqlite3_stmt* stmt = prepare(sqlite->db, SELECT_ACCNT_SQL);
    if (!stmt)
        goto fail1;

    struct DbrQueue rq;
    dbr_queue_init(&rq);

    size_t size = 0;
    for (;;) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {

            struct DbrRec* rec = dbr_pool_alloc_rec(pool);
            if (!rec)
                goto fail2;
            dbr_rec_init(rec);

            // Header.

            rec->type = DBR_ENTITY_ACCNT;
            rec->id = sqlite3_column_int64(stmt, ID);
            strncpy(rec->mnem,
                    (const char*)sqlite3_column_text(stmt, MNEM), DBR_MNEM_MAX);
            strncpy(rec->display,
                    (const char*)sqlite3_column_text(stmt, DISPLAY), DBR_DISPLAY_MAX);

            // Body.

            strncpy(rec->accnt.email,
                    (const char*)sqlite3_column_text(stmt, EMAIL), DBR_EMAIL_MAX);
            rec->accnt.state = NULL;

            dbr_log_debug3("accnt: id=%ld,mnem=%.16s,display=%.64s,email=%.64s",
                           rec->id, rec->mnem, rec->display, rec->accnt.email);

            dbr_queue_insert_back(&rq, &rec->shared_node_);
            ++size;

        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            dbr_err_set(DBR_EIO, sqlite3_errmsg(sqlite->db));
            goto fail2;
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    *first = dbr_queue_first(&rq);
    return size;
 fail2:
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    dbr_pool_free_entity_list(pool, DBR_ENTITY_ACCNT, dbr_queue_first(&rq));
    *first = NULL;
 fail1:
    return -1;
}

static ssize_t
select_contr(struct FirSqlite* sqlite, DbrPool pool, struct DbrSlNode** first)
{
    enum {
        ID,
        MNEM,
        DISPLAY,
        ASSET_TYPE,
        ASSET,
        CCY,
        TICK_NUMER,
        TICK_DENOM,
        LOT_NUMER,
        LOT_DENOM,
        PIP_DP,
        MIN_LOTS,
        MAX_LOTS
    };

    sqlite3_stmt* stmt = prepare(sqlite->db, SELECT_CONTR_SQL);
    if (!stmt)
        goto fail1;

    struct DbrQueue rq;
    dbr_queue_init(&rq);

    ssize_t size = 0;
    for (;;) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {

            struct DbrRec* rec = dbr_pool_alloc_rec(pool);
            if (!rec)
                goto fail2;
            dbr_rec_init(rec);

            // Header.

            rec->type = DBR_ENTITY_CONTR;
            rec->id = sqlite3_column_int64(stmt, ID);
            strncpy(rec->mnem,
                    (const char*)sqlite3_column_text(stmt, MNEM), DBR_MNEM_MAX);
            strncpy(rec->display,
                    (const char*)sqlite3_column_text(stmt, DISPLAY), DBR_DISPLAY_MAX);

            // Body.

            strncpy(rec->contr.asset_type,
                    (const char*)sqlite3_column_text(stmt, ASSET_TYPE), DBR_MNEM_MAX);
            strncpy(rec->contr.asset,
                    (const char*)sqlite3_column_text(stmt, ASSET), DBR_MNEM_MAX);
            strncpy(rec->contr.ccy,
                    (const char*)sqlite3_column_text(stmt, CCY), DBR_MNEM_MAX);

            const int tick_numer = sqlite3_column_int(stmt, TICK_NUMER);
            const int tick_denom = sqlite3_column_int(stmt, TICK_DENOM);
            const double price_inc = dbr_fract_to_real(tick_numer, tick_denom);
            rec->contr.tick_numer = tick_numer;
            rec->contr.tick_denom = tick_denom;
            rec->contr.price_inc = price_inc;

            const int lot_numer = sqlite3_column_int(stmt, LOT_NUMER);
            const int lot_denom = sqlite3_column_int(stmt, LOT_DENOM);
            const double qty_inc = dbr_fract_to_real(lot_numer, lot_denom);
            rec->contr.lot_numer = lot_numer;
            rec->contr.lot_denom = lot_denom;
            rec->contr.qty_inc = qty_inc;

            rec->contr.price_dp = dbr_real_to_dp(price_inc);
            rec->contr.pip_dp = sqlite3_column_int(stmt, PIP_DP);
            rec->contr.qty_dp = dbr_real_to_dp(qty_inc);

            rec->contr.min_lots = sqlite3_column_int64(stmt, MIN_LOTS);
            rec->contr.max_lots = sqlite3_column_int64(stmt, MAX_LOTS);

            dbr_log_debug3("contr: id=%ld,mnem=%.16s,display=%.64s,asset_type=%.16s,"
                           "asset=%.16s,ccy=%.16s,price_inc=%f,qty_inc=%.2f,price_dp=%d,"
                           "pip_dp=%d,qty_dp=%d,min_lots=%ld,max_lots=%ld",
                           rec->id, rec->mnem, rec->display, rec->contr.asset_type,
                           rec->contr.asset, rec->contr.ccy, rec->contr.price_inc,
                           rec->contr.qty_inc, rec->contr.price_dp, rec->contr.pip_dp,
                           rec->contr.qty_dp, rec->contr.min_lots, rec->contr.max_lots);

            dbr_queue_insert_back(&rq, &rec->shared_node_);
            ++size;

        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            dbr_err_set(DBR_EIO, sqlite3_errmsg(sqlite->db));
            goto fail2;
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    *first = dbr_queue_first(&rq);
    return size;
 fail2:
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    dbr_pool_free_entity_list(pool, DBR_ENTITY_CONTR, dbr_queue_first(&rq));
    *first = NULL;
 fail1:
    return -1;
}

static ssize_t
select_order(struct FirSqlite* sqlite, DbrPool pool, struct DbrSlNode** first)
{
    enum {
        ID,
        TRADER,
        ACCNT,
        CONTR,
        SETTL_DATE,
        REF,
        STATE,
        ACTION,
        TICKS,
        LOTS,
        RESD,
        EXEC,
        LAST_TICKS,
        LAST_LOTS,
        MIN_LOTS,
        CREATED,
        MODIFIED
    };

    sqlite3_stmt* stmt = prepare(sqlite->db, SELECT_ORDER_SQL);
    if (!stmt)
        goto fail1;

    struct DbrQueue oq;
    dbr_queue_init(&oq);

    ssize_t size = 0;
    for (;;) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {

            struct DbrOrder* order = dbr_pool_alloc_order(pool);
            if (!order)
                goto fail2;
            dbr_order_init(order);

            order->level = NULL;
            order->id = sqlite3_column_int64(stmt, ID);
            order->c.trader.id_only = sqlite3_column_int64(stmt, TRADER);
            order->c.accnt.id_only = sqlite3_column_int64(stmt, ACCNT);
            order->c.contr.id_only = sqlite3_column_int64(stmt, CONTR);
            order->c.settl_date = sqlite3_column_int(stmt, SETTL_DATE);
            if (sqlite3_column_type(stmt, REF) != SQLITE_NULL)
                strncpy(order->c.ref,
                        (const char*)sqlite3_column_text(stmt, REF), DBR_REF_MAX);
            else
                order->c.ref[0] = '\0';
            order->c.state = sqlite3_column_int(stmt, STATE);
            order->c.action = sqlite3_column_int(stmt, ACTION);
            order->c.ticks = sqlite3_column_int64(stmt, TICKS);
            order->c.lots = sqlite3_column_int64(stmt, LOTS);
            order->c.resd = sqlite3_column_int64(stmt, RESD);
            order->c.exec = sqlite3_column_int64(stmt, EXEC);
            order->c.last_ticks = sqlite3_column_int64(stmt, LAST_TICKS);
            order->c.last_lots = sqlite3_column_int64(stmt, LAST_LOTS);
            order->c.min_lots = sqlite3_column_int64(stmt, MIN_LOTS);
            order->created = sqlite3_column_int64(stmt, CREATED);
            order->modified = sqlite3_column_int64(stmt, MODIFIED);

            dbr_log_debug3("order: id=%ld,trader=%ld,accnt=%ld,contr=%ld,settl_date=%d,"
                           "ref=%.64s,state=%d,action=%d,ticks=%ld,lots=%ld,resd=%ld,"
                           "exec=%ld,last_ticks=%ld,last_lots=%ld,min_lots=%ld,"
                           "created=%ld,modified=%ld",
                           order->id, order->c.trader.id_only, order->c.accnt.id_only,
                           order->c.contr.id_only, order->c.settl_date, order->c.ref,
                           order->c.state, order->c.action, order->c.ticks, order->c.lots,
                           order->c.resd, order->c.exec, order->c.last_ticks, order->c.last_lots,
                           order->c.min_lots, order->created, order->modified);

            dbr_queue_insert_back(&oq, &order->shared_node_);
            ++size;

        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            dbr_err_set(DBR_EIO, sqlite3_errmsg(sqlite->db));
            goto fail2;
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    *first = dbr_queue_first(&oq);
    return size;
 fail2:
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    dbr_pool_free_entity_list(pool, DBR_ENTITY_ORDER, dbr_queue_first(&oq));
    *first = NULL;
 fail1:
    return -1;
}

static ssize_t
select_trade(struct FirSqlite* sqlite, DbrPool pool, struct DbrSlNode** first)
{
    enum {
        ID,
        ORDER,
        TRADER,
        ACCNT,
        CONTR,
        SETTL_DATE,
        REF,
        ACTION,
        TICKS,
        LOTS,
        RESD,
        EXEC,
        LAST_TICKS,
        LAST_LOTS,
        MIN_LOTS,
        MATCH,
        ROLE,
        CPTY,
        CREATED
    };

    sqlite3_stmt* stmt = prepare(sqlite->db, SELECT_TRADE_SQL);
    if (!stmt)
        goto fail1;

    struct DbrQueue tq;
    dbr_queue_init(&tq);

    ssize_t size = 0;
    for (;;) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {

            struct DbrExec* exec = dbr_pool_alloc_exec(pool);
            if (!exec)
                goto fail2;
            dbr_exec_init(exec);

            exec->id = sqlite3_column_int64(stmt, ID);
            exec->order = sqlite3_column_int64(stmt, ORDER);
            exec->c.trader.id_only = sqlite3_column_int64(stmt, TRADER);
            exec->c.accnt.id_only = sqlite3_column_int64(stmt, ACCNT);
            exec->c.contr.id_only = sqlite3_column_int64(stmt, CONTR);
            exec->c.settl_date = sqlite3_column_int(stmt, SETTL_DATE);
            if (sqlite3_column_type(stmt, REF) != SQLITE_NULL)
                strncpy(exec->c.ref,
                        (const char*)sqlite3_column_text(stmt, REF), DBR_REF_MAX);
            else
                exec->c.ref[0] = '\0';
            exec->c.state = DBR_STATE_TRADE;
            exec->c.action = sqlite3_column_int(stmt, ACTION);
            exec->c.ticks = sqlite3_column_int64(stmt, TICKS);
            exec->c.lots = sqlite3_column_int64(stmt, LOTS);
            exec->c.resd = sqlite3_column_int64(stmt, RESD);
            exec->c.exec = sqlite3_column_int64(stmt, EXEC);
            exec->c.last_ticks = sqlite3_column_int64(stmt, LAST_TICKS);
            exec->c.last_lots = sqlite3_column_int64(stmt, LAST_LOTS);
            exec->c.min_lots = sqlite3_column_int64(stmt, MIN_LOTS);
            exec->match = sqlite3_column_type(stmt, MATCH) != SQLITE_NULL
                ? sqlite3_column_int64(stmt, MATCH) : 0;
            exec->role = sqlite3_column_type(stmt, ROLE) != SQLITE_NULL
                ? sqlite3_column_int(stmt, ROLE) : 0;
            exec->cpty.id_only = sqlite3_column_type(stmt, CPTY) != SQLITE_NULL
                ? sqlite3_column_int64(stmt, CPTY) : 0;
            exec->created = sqlite3_column_int64(stmt, CREATED);

            dbr_log_debug3("exec: id=%ld,order=%ld,trader=%ld,accnt=%ld,contr=%ld,settl_date=%d,"
                           "ref=%.64s,action=%d,ticks=%ld,lots=%ld,resd=%ld,exec=%ld,"
                           "last_ticks=%ld,last_lots=%ld,min_lots=%ld,match=%ld,role=%d,cpty=%ld,"
                           "created=%ld",
                           exec->id, exec->order, exec->c.trader.id_only, exec->c.accnt.id_only,
                           exec->c.contr.id_only, exec->c.settl_date, exec->c.ref, exec->c.action,
                           exec->c.ticks, exec->c.lots, exec->c.resd, exec->c.exec,
                           exec->c.last_ticks, exec->c.last_lots, exec->c.min_lots,
                           exec->match, exec->role, exec->cpty.id_only, exec->created);

            dbr_queue_insert_back(&tq, &exec->shared_node_);
            ++size;

        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            dbr_err_set(DBR_EIO, sqlite3_errmsg(sqlite->db));
            goto fail2;
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    *first = dbr_queue_first(&tq);
    return size;
 fail2:
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    dbr_pool_free_entity_list(pool, DBR_ENTITY_EXEC, dbr_queue_first(&tq));
    *first = NULL;
 fail1:
    return -1;
}

static ssize_t
select_memb(struct FirSqlite* sqlite, DbrPool pool, struct DbrSlNode** first)
{
    enum {
        ACCNT,
        TRADER
    };

    sqlite3_stmt* stmt = prepare(sqlite->db, SELECT_MEMB_SQL);
    if (!stmt)
        goto fail1;

    struct DbrQueue mq;
    dbr_queue_init(&mq);

    ssize_t size = 0;
    for (;;) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {

            struct DbrMemb* memb = dbr_pool_alloc_memb(pool);
            if (!memb)
                goto fail2;
            dbr_memb_init(memb);

            memb->accnt.id_only = sqlite3_column_int64(stmt, ACCNT);
            memb->trader.id_only = sqlite3_column_int64(stmt, TRADER);

            dbr_log_debug3("memb: accnt=%ld,trader=%ld",
                           memb->accnt.id_only, memb->trader.id_only);

            dbr_queue_insert_back(&mq, &memb->shared_node_);
            ++size;

        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            dbr_err_set(DBR_EIO, sqlite3_errmsg(sqlite->db));
            goto fail2;
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    *first = dbr_queue_first(&mq);
    return size;
 fail2:
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    dbr_pool_free_entity_list(pool, DBR_ENTITY_MEMB, dbr_queue_first(&mq));
    *first = NULL;
 fail1:
    return -1;
}

static ssize_t
select_posn(struct FirSqlite* sqlite, DbrPool pool, struct DbrSlNode** first)
{
    enum {
        ACCNT,
        CONTR,
        SETTL_DATE,
        ACTION,
        LICKS,
        LOTS
    };

    sqlite3_stmt* stmt = prepare(sqlite->db, SELECT_POSN_SQL);
    if (!stmt)
        goto fail1;

    struct DbrQueue pq;
    dbr_queue_init(&pq);

    struct DbrPosn* posn = NULL;

    ssize_t size = 0;
    for (;;) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {

            const DbrIden accnt = sqlite3_column_int64(stmt, ACCNT);
            const DbrIden contr = sqlite3_column_int64(stmt, CONTR);
            const DbrDate settl_date = sqlite3_column_int(stmt, SETTL_DATE);

            // Posn is null for first row.
            if (posn && posn->accnt.id_only == accnt && posn->contr.id_only == contr
                && posn->settl_date == settl_date) {

                // Set other side.

                const int action = sqlite3_column_int(stmt, ACTION);
                if (action == DBR_ACTION_BUY) {
                    posn->buy_licks = sqlite3_column_int64(stmt, LICKS);
                    posn->buy_lots = sqlite3_column_int64(stmt, LOTS);
                } else {
                    assert(action == DBR_ACTION_SELL);
                    posn->sell_licks = sqlite3_column_int64(stmt, LICKS);
                    posn->sell_lots = sqlite3_column_int64(stmt, LOTS);
                }
                continue;
            }

            posn = dbr_pool_alloc_posn(pool);
            if (dbr_unlikely(!posn))
                goto fail2;
            dbr_posn_init(posn);

            posn->accnt.id_only = accnt;
            posn->contr.id_only = contr;
            posn->settl_date = settl_date;

            const int action = sqlite3_column_int(stmt, ACTION);
            if (action == DBR_ACTION_BUY) {
                posn->buy_licks = sqlite3_column_int64(stmt, LICKS);
                posn->buy_lots = sqlite3_column_int64(stmt, LOTS);
                posn->sell_licks = 0;
                posn->sell_lots = 0;
            } else {
                assert(action == DBR_ACTION_SELL);
                posn->buy_licks = 0;
                posn->buy_lots = 0;
                posn->sell_licks = sqlite3_column_int64(stmt, LICKS);
                posn->sell_lots = sqlite3_column_int64(stmt, LOTS);
            }

            dbr_log_debug3("posn: accnt=%ld,contr=%ld,settl_date=%d,buy_licks=%ld,buy_lots=%ld,"
                           "sell_licks=%ld,sell_lots=%ld",
                           posn->accnt.id_only, posn->contr.id_only, posn->settl_date,
                           posn->buy_licks, posn->buy_lots, posn->sell_licks, posn->sell_lots);

            dbr_queue_insert_back(&pq, &posn->shared_node_);
            ++size;

        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            dbr_err_set(DBR_EIO, sqlite3_errmsg(sqlite->db));
            goto fail2;
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    *first = dbr_queue_first(&pq);
    return size;
 fail2:
    sqlite3_clear_bindings(stmt);
    sqlite3_finalize(stmt);
    dbr_pool_free_entity_list(pool, DBR_ENTITY_POSN, dbr_queue_first(&pq));
    *first = NULL;
 fail1:
    return -1;
}

DBR_EXTERN DbrBool
fir_sqlite_init(struct FirSqlite* sqlite, const char* path)
{
    sqlite3* db;
    int rc = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE, NULL);
    if (dbr_unlikely(rc != SQLITE_OK)) {
        dbr_err_set(DBR_EIO, sqlite3_errmsg(db));
        // Must close even if open failed.
        goto fail1;
    }

    // Install trace function for debugging.
#if DBR_DEBUG_LEVEL >= 2
    sqlite3_trace(db, trace_sql, NULL);
#endif // DBR_DEBUG_LEVEL >= 2

#if DBR_DEBUG_LEVEL >= 1
    rc = sqlite3_db_config(db, SQLITE_DBCONFIG_ENABLE_FKEY, 1, NULL);
    if (dbr_unlikely(rc != SQLITE_OK)) {
        dbr_err_set(DBR_EIO, sqlite3_errmsg(db));
        goto fail1;
    }
#endif // DBR_DEBUG_LEVEL >= 1

    if (dbr_unlikely(!exec_sql(db, "PRAGMA synchronous = OFF")
                     || !exec_sql(db, "PRAGMA journal_mode = MEMORY"))) {
        dbr_err_set(DBR_EIO, sqlite3_errmsg(db));
        goto fail1;
    }

    sqlite3_stmt* insert_exec = prepare(db, INSERT_EXEC_SQL);
    if (!insert_exec)
        goto fail1;

    sqlite3_stmt* update_exec = prepare(db, UPDATE_EXEC_SQL);
    if (!update_exec)
        goto fail2;

    sqlite->db = db;
    sqlite->insert_exec = insert_exec;
    sqlite->update_exec = update_exec;
    return DBR_TRUE;
 fail2:
    sqlite3_finalize(insert_exec);
 fail1:
    sqlite3_close(db);
    return DBR_FALSE;
}

DBR_EXTERN void
fir_sqlite_term(struct FirSqlite* sqlite)
{
    assert(sqlite);
    sqlite3_finalize(sqlite->update_exec);
    sqlite3_finalize(sqlite->insert_exec);
    sqlite3_close(sqlite->db);
}

DBR_EXTERN DbrBool
fir_sqlite_begin_trans(struct FirSqlite* sqlite)
{
    return exec_sql(sqlite->db, "BEGIN TRANSACTION");
}

DBR_EXTERN DbrBool
fir_sqlite_commit_trans(struct FirSqlite* sqlite)
{
    return exec_sql(sqlite->db, "COMMIT TRANSACTION");
}

DBR_EXTERN DbrBool
fir_sqlite_rollback_trans(struct FirSqlite* sqlite)
{
    return exec_sql(sqlite->db, "ROLLBACK TRANSACTION");
}

DBR_EXTERN DbrBool
fir_sqlite_insert_exec(struct FirSqlite* sqlite, const struct DbrExec* exec, DbrBool enriched)
{
    return bind_insert_exec(sqlite, exec, enriched)
        && exec_stmt(sqlite->db, sqlite->insert_exec);
}

DBR_EXTERN DbrBool
fir_sqlite_update_exec(struct FirSqlite* sqlite, DbrIden id, DbrMillis modified)
{
    return bind_update_exec(sqlite, id, modified)
        && exec_stmt(sqlite->db, sqlite->update_exec);
}

DBR_EXTERN ssize_t
fir_sqlite_select_entity(struct FirSqlite* sqlite, int type, DbrPool pool,
                         struct DbrSlNode** first)
{
    ssize_t ret;
    switch (type) {
    case DBR_ENTITY_TRADER:
        ret = select_trader(sqlite, pool, first);
        break;
    case DBR_ENTITY_ACCNT:
        ret = select_accnt(sqlite, pool, first);
        break;
    case DBR_ENTITY_CONTR:
        ret = select_contr(sqlite, pool, first);
        break;
    case DBR_ENTITY_ORDER:
        ret = select_order(sqlite, pool, first);
        break;
    case DBR_ENTITY_EXEC:
        ret = select_trade(sqlite, pool, first);
        break;
    case DBR_ENTITY_MEMB:
        ret = select_memb(sqlite, pool, first);
        break;
    case DBR_ENTITY_POSN:
        ret = select_posn(sqlite, pool, first);
        break;
    default:
        dbr_err_setf(DBR_EINVAL, "invalid type '%d'", type);
        *first = NULL;
        ret = -1;
    }
    return ret;
}

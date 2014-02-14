cimport dbr
cimport zmq

cdef extern from "dbrpy/dbrpy.h":

    ctypedef dbr.DbrExec DbrpyExec
    ctypedef dbr.DbrPosn DbrpyPosn
    ctypedef dbr.DbrView DbrpyView

    ctypedef dbr.DbrHandlerVtbl DbrpyHandlerVtbl
    ctypedef dbr.DbrIHandler DbrpyIHandler

from inspect import currentframe, getframeinfo

EINTR = dbr.DBR_EINTR
EIO = dbr.DBR_EIO
ENOMEM = dbr.DBR_ENOMEM
EACCES = dbr.DBR_EACCES
EBUSY = dbr.DBR_EBUSY
EEXIST = dbr.DBR_EEXIST
EINVAL = dbr.DBR_EINVAL
ETIMEOUT = dbr.DBR_ETIMEOUT
EUSER = dbr.DBR_EUSER

def err_set(int num, const char* msg):
    fi = getframeinfo(currentframe())
    dbr.dbr_err_set_(num, fi.filename, fi.lineno, msg)

def err_num():
    return dbr.dbr_err_num()

def err_file():
    return dbr.dbr_err_file()

def err_line():
    return dbr.dbr_err_line()

def err_msg():
    return dbr.dbr_err_msg()

class Error(Exception):
    def __init__(self):
        self.num = err_num()
        self.file = err_file()
        self.line = err_line()
        self.msg = err_msg()
    def __str__(self):
        return repr("{1}:{2}: {3} ({0})".format(self.num, self.file, self.line, self.msg))

def millis():
    return dbr.dbr_millis()

cdef class ZmqCtx:
    cdef void* impl_

    def __cinit__(self):
        self.impl_ = zmq.zmq_ctx_new()
        if self.impl_ is NULL:
            fi = getframeinfo(currentframe())
            dbr.dbr_err_setf_(dbr.DBR_EIO, fi.filename, fi.lineno,
                              "zmq_ctx_new() failed: %s", zmq.zmq_strerror(zmq.zmq_errno()))
            raise Error()

    def __dealloc__(self):
        if self.impl_ is not NULL:
            zmq.zmq_ctx_destroy(self.impl_)

cdef class Pool:
    cdef dbr.DbrPool impl_

    def __cinit__(self, capacity):
        self.impl_ = dbr.dbr_pool_create(capacity)
        if self.impl_ is NULL:
            raise Error()

    def __dealloc__(self):
        if self.impl_ is not NULL:
            dbr.dbr_pool_destroy(self.impl_)

cdef struct HandlerImpl:
    void* target
    DbrpyIHandler handler

cdef inline void* handler_target(dbr.DbrHandler handler):
    cdef size_t offset = <size_t>&(<HandlerImpl*>NULL).handler
    cdef HandlerImpl* impl = <HandlerImpl*>(<char*>handler - offset)
    return impl.target

cdef void on_up(dbr.DbrHandler handler, int conn):
    (<object>handler_target(handler)).on_up(conn)

cdef void on_down(dbr.DbrHandler handler, int conn):
    (<object>handler_target(handler)).on_down()

cdef void on_logon(dbr.DbrHandler handler, dbr.DbrIden tid):
    (<object>handler_target(handler)).on_logon()

cdef void on_logoff(dbr.DbrHandler handler, dbr.DbrIden tid):
    (<object>handler_target(handler)).on_logoff()

cdef void on_timeout(dbr.DbrHandler handler, dbr.DbrIden req_id):
    (<object>handler_target(handler)).on_timeout()

cdef void on_status(dbr.DbrHandler handler, dbr.DbrIden req_id, int num, const char* msg):
    (<object>handler_target(handler)).on_status()

cdef void on_exec(dbr.DbrHandler handler, dbr.DbrIden req_id, DbrpyExec* exc):
    (<object>handler_target(handler)).on_exec()

cdef void on_posn(dbr.DbrHandler handler, DbrpyPosn* posn):
    (<object>handler_target(handler)).on_posn()

cdef void on_view(dbr.DbrHandler handler, DbrpyView* view):
    (<object>handler_target(handler)).on_view()

cdef void on_flush(dbr.DbrHandler handler):
    (<object>handler_target(handler)).on_flush()

cdef class Handler:
    cdef DbrpyHandlerVtbl vtbl_
    cdef HandlerImpl impl_

    def __init__(self):
        self.vtbl_.on_up = on_up
        self.vtbl_.on_down = on_down
        self.vtbl_.on_logon = on_logon
        self.vtbl_.on_logoff = on_logoff
        self.vtbl_.on_timeout = on_timeout
        self.vtbl_.on_status = on_status
        self.vtbl_.on_exec = on_exec
        self.vtbl_.on_posn = on_posn
        self.vtbl_.on_view = on_view
        self.vtbl_.on_flush = on_flush
        self.impl_.target = <void*>self
        self.impl_.handler.vtbl = &self.vtbl_

cdef class Clnt:
    cdef dbr.DbrClnt impl_

    def __cinit__(self, const char* sess, ZmqCtx ctx, const char* mdaddr,
                  const char* traddr, dbr.DbrIden seed, Pool pool):
        self.impl_ = dbr.dbr_clnt_create(sess, ctx.impl_, mdaddr, traddr, seed, pool.impl_)
        if self.impl_ is NULL:
            raise Error()

    def __dealloc__(self):
        if self.impl_ is not NULL:
            dbr.dbr_clnt_destroy(self.impl_)

    def close(self, dbr.DbrMillis ms):
        if dbr.dbr_clnt_close(self.impl_, ms) == -1:
            raise Error()

    def is_open(self):
        return <bint>dbr.dbr_clnt_is_open(self.impl_)

    def is_ready(self):
        return <bint>dbr.dbr_clnt_is_ready(self.impl_)

    def poll(self, dbr.DbrMillis ms, Handler handler):
        return dbr.dbr_clnt_poll(self.impl_, ms, &handler.impl_.handler)
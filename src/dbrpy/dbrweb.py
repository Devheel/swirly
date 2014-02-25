#!/usr/bin/env python

from dbrpy import *
from threading import Thread

import json
import time
import web

SESS = 'TEST'
TMOUT = 2000

class WorkerHandler(Handler):
    def __init__(self, clnt):
        super(WorkerHandler, self).__init__()
        self.clnt = clnt
    def on_async(self, fn):
        return fn(self.clnt)

def worker(ctx):
    pool = Pool(8 * 1024 * 1024)
    clnt = Clnt(ctx, SESS, 'tcp://localhost:3270', 'tcp://localhost:3271',
                millis(), pool)
    handler = WorkerHandler(clnt)
    while clnt.is_open():
        clnt.poll(TMOUT, handler)

class CloseRequest(object):
    def __call__(self, clnt):
        return clnt.close(TMOUT)

class TraderRequest(object):
    @staticmethod
    def traderDict(trec):
        return {
            'id': trec.id,
            'mnem': trec.mnem,
            'display': trec.display,
            'email': trec.email
        }
    def __init__(self, mnems):
        self.mnems = mnems
    def __call__(self, clnt):
        traders = []
        if self.mnems:
            for mnem in self.mnems:
                trec = clnt.find_rec_mnem(ENTITY_TRADER, mnem)
                if trec:
                    traders.append(TraderRequest.traderDict(trec))
        else:
            traders = [TraderRequest.traderDict(trec)
                       for trec in clnt.list_rec(ENTITY_TRADER)]
        return traders

class AccntRequest(object):
    @staticmethod
    def accntDict(arec):
        return {
            'id': arec.id,
            'mnem': arec.mnem,
            'display': arec.display,
            'email': arec.email
        }
    def __init__(self, mnems):
        self.mnems = mnems
    def __call__(self, clnt):
        accnts = []
        if self.mnems:
            for mnem in self.mnems:
                arec = clnt.find_rec_mnem(ENTITY_ACCNT, mnem)
                if arec:
                    accnts.append(AccntRequest.accntDict(arec))
        else:
            accnts = [AccntRequest.accntDict(arec)
                      for arec in clnt.list_rec(ENTITY_ACCNT)]
        return accnts

class ContrRequest(object):
    @staticmethod
    def contrDict(arec):
        return {
            'id': arec.id,
            'mnem': arec.mnem,
            'display': arec.display
        }
    def __init__(self, mnems):
        self.mnems = mnems
    def __call__(self, clnt):
        contrs = []
        if self.mnems:
            for mnem in self.mnems:
                arec = clnt.find_rec_mnem(ENTITY_CONTR, mnem)
                if arec:
                    contrs.append(ContrRequest.contrDict(arec))
        else:
            contrs = [ContrRequest.contrDict(arec)
                      for arec in clnt.list_rec(ENTITY_CONTR)]
        return contrs

urls = (
    '/api/trader', 'TraderHandler',
    '/api/accnt',  'AccntHandler',
    '/api/contr',  'ContrHandler'
)

class TraderHandler:
    def GET(self):
        async = web.ctx.async
        async.send(TraderRequest(web.input(mnem = []).mnem))
        web.header('Content-Type', 'application/json')
        return json.dumps(async.recv())

class AccntHandler:
    def GET(self):
        async = web.ctx.async
        async.send(AccntRequest(web.input(mnem = []).mnem))
        web.header('Content-Type', 'application/json')
        return json.dumps(async.recv())

class ContrHandler:
    def GET(self):
        async = web.ctx.async
        async.send(ContrRequest(web.input(mnem = []).mnem))
        web.header('Content-Type', 'application/json')
        return json.dumps(async.recv())

def run():
    ctx = ZmqCtx()
    thread = Thread(target = worker, args = (ctx,))
    thread.start()
    async = Async(ctx, SESS)
    app = web.application(urls, globals())
    def loadhook():
        web.ctx.async = async
    app.add_processor(web.loadhook(loadhook))
    app.run()
    print('shutdown')
    async.send(CloseRequest())
    async.recv()
    thread.join()

if __name__ == '__main__':
    try:
        run()
    except Error as e:
        print 'error: ' + str(e)
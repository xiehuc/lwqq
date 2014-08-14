#!/usr/bin/python3
#python2 compible
from __future__ import unicode_literals

from lwqq.types import *
from lwqq.vplist import *
from lwqq.lwqq import *
from lwqq.core import Event,Evset
from lwqq import core
from lwqq import lwjs
from lwqq.msg import *
from ctypes import c_voidp,cast,POINTER,c_int,byref,c_char_p,pointer,CFUNCTYPE,py_object
from tornado.ioloop import IOLoop
from os import read
from six import print_,u
from six.moves.urllib import request
import functools
import sys
import argparse
import lwqq

loop = IOLoop.instance()
lc = None 
prompt_hint = '>>>'

class ArgsParser(argparse.ArgumentParser):
    def error(self, message):
        print_(message,file=sys.stderr)

#return a python str
def utf(x):
    return x.decode('utf-8')
#return a c_str
def cstr(x):
    return x.encode('utf-8')

def prompt(hint='>>>'):
    print_(hint,end='')
    sys.stdout.flush()

def start_login(p_login_ec):
    login_ec = cast(p_login_ec,POINTER(c_int))[0]
    print_("login_ec",login_ec)
    print_("===start_login===")

def need_verify(p_vf_image):
    vf = core.VerifyCode(cast(p_vf_image,POINTER(core.VerifyCode.PT))[0])
    path = "/tmp/"+utf(lc.username)+".jpg"
    vf.save(path)
    print_("need verify image, see: "+path)
    code = input("Input:")
    vf.input(cstr(code))
    pass

def message_cb():
    for msg in lc.msg_list.read():
        print_(msg.typeid)
        if msg.trycast(Message):
            m = Message(msg.ref)
            print_(m.sender)
            print_(str(m))
        msg.destroy()
    prompt(prompt_hint)

def message_lost():
    lc.logout()
    exit(0)

def init_listener(lc):
    lc.addListener(lc.events.start_login,Command.make('p',start_login,lc.args.login_ec))
    lc.addListener(lc.events.need_verify,Command.make('p',need_verify,lc.args.vf_image))
    lc.addListener(lc.events.login_complete,load_info)
    lc.addListener(lc.events.poll_msg,message_cb)
    lc.addListener(lc.events.poll_lost,message_lost)

def load_info():
    if not core.has_feature(Features.WITH_MOZJS):
        lc.get_friends_info(None,None).addListener(load_group_info)
    else:
        js = lwjs.Lwjs()
        hashjs = request.urlopen("http://pidginlwqq.sinaapp.com/hash.js")
        js.load(hashjs.read())
        lc.get_friends_info(js.hashfunc,js.js).addListener(load_group_info)
    pass

def load_group_info():
    ev = Evset.new()
    lc.get_group_list().addto(ev)
    lc.get_discu_list().addto(ev)
    ev.addListener(poll_msg)

def print_help():
    print_("""avaliable command:
    help --- show this help
    ls   --- list friends and groups
    talk --- talk to friends
    quit --- quit program
    """)

def poll_msg():
    lc.msg_list.poll(0)
    print_help()
    prompt()

def ignore():
    return

ls = ArgsParser(prog='ls',description='list friends',add_help=False)
ls.add_argument('who',help='friends uin',nargs='?')
ls.add_argument('-h',help='print help',action='store_true')

def list_friend(argv):
    args = ls.parse_args(argv)
    if args.h:
        ls.print_help()
        return
    if not args.who:
        for b in lc.friends():
            print_('[Buddy uin:{} nick:{}]'.format(b.uin,utf(b.nick)))
        print_()
        for g in lc.groups():
            print_('[Group gid:{} name:{}]'.format(g.gid,utf(g.name)))
        print_()
        for d in lc.discus():
            print_('[Discu did:{} name:{}]'.format(d.did,utf(d.name)))
        return
    b = lc.find_buddy(uin=cstr(args.who))
    g = lc.find_group(gid=cstr(args.who))
    d = lc.find_discu(did=cstr(args.who))
    if not b and not g and not d:
        print_("not found")
        return
    found = b if b else g if g else d
    if not found.qqnumber:
        found.get_qqnumber()
        found.get_detail().addListener(
                functools.partial(list_friend,argv)
                )
        print_("waiting for a while, fetch from server")
        return
    if b:
        c = b.get_category()
        if c: print_('in Category:'+utf(c.name))
    print_(str(b))

quit = ArgsParser(prog='quit',description='quit program',add_help=False)
quit.add_argument('-h',help='print help',action='store_true')

def quit_program(argv):
    args = quit.parse_args(argv)
    if args.h:
        quit.print_help()
        return
    lc.logout()
    quit.exit()

talk = ArgsParser(prog='talk', description='talk to some one',add_help=False)
talk.add_argument('to', help='some one you want to talk')
talk_tg = None


def talk_mode(fd,events):
    global prompt_hint
    print_()
    send = read(fd,512)
    if not send :
        loop.remove_handler(0)
        loop.add_handler(0,command,loop.READ)
        prompt_hint = '>>>'
        prompt(prompt_hint)
        return
    send = send[:-1]
    txt = Text.new(text=send)
    BuddyMessage.new().append(txt).send(talk_tg)
    prompt("Input:")

def talk_to(argv):
    global talk_tg
    args = talk.parse_args(argv)
    if args.to:
        b = lc.find_buddy(uin=cstr(args.to),nick=cstr(args.to))
        if b:
            global prompt_hint
            prompt_hint = 'Input:'
            loop.remove_handler(0)
            loop.add_handler(0,talk_mode,loop.READ)
            talk_tg = b
            print_('Use Ctrl-D to end talk')
            prompt(prompt_hint)

def command(fd,events):
    argv = utf(read(fd,100)).rstrip().split(' ')
    if argv[0]=='ls' or argv[0] == 'l':
        list_friend(argv[1:])
    elif argv[0]=='quit' or argv[0]=='q':
        quit_program(argv[1:])
    elif argv[0]=='talk' or argv[0]=='t':
        talk_to(argv[1:])
    elif argv[0]=='help' or argv[0]=='h':
        print_help()
    print_()
    prompt()

def local_thread(cmd):
    cmd.invoke()
    pass

def dispatch(cmd,timeout):
    loop.add_callback(local_thread,cmd)
    pass

def main():
    lc.login(Status.ONLINE)
    pass

def show_version():
    print('version:',utf(lwqq.lwqq.version))
    feature = {
            Features.WITH_LIBEV: "libev",
            Features.WITH_LIBUV: "libuv",
            Features.WITH_MOZJS: "mozjs",
            Features.WITH_SQLITE: "sqlite",
            Features.WITH_SSL: "ssl"
            }
    print('feature:',[feature[x] for x in lwqq.lwqq.feature])
    pass

argp = argparse.ArgumentParser(description = 'command line tool to talk with qq friend')
arg1 = argp.add_argument_group()
arg1.add_argument('user',help='username')
arg1.add_argument('password',help='password')
arg1.add_argument('-v',help='verbose level',action='count')
arg2 = argp.add_argument_group()
arg2.add_argument('--version', help='print version info', action='store_true')

if __name__ == '__main__':
    args = argp.parse_args()

    if args.version:
        show_version()
        exit(0)
    lc = Lwqq(cstr(args.user),cstr(args.password))
    if args.verbose:
        Lwqq.log_level(args.verbose)
    lc.setDispatcher(dispatch)
    init_listener(lc)
    loop.add_callback(main)
    loop.add_handler(0,command,loop.READ)
    loop.start()


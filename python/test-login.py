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
import functools
import urllib.request
import sys
import argparse

loop = IOLoop.instance()
lc = Lwqq(b'2501542492',b'1234567890+12345')

class ArgsParser(argparse.ArgumentParser):
    def error(self, message):
        print(message,file=sys.stderr)

def prompt(hint='>>>'):
    print(hint,end='')
    sys.stdout.flush()

def start_login(p_login_ec):
    login_ec = cast(p_login_ec,POINTER(c_int))[0]
    print("login_ec",login_ec)
    print("===start_login===")

def need_verify(p_vf_image):
    vf = core.VerifyCode(cast(p_vf_image,POINTER(core.VerifyCode.PT))[0])
    path = "/tmp/"+lc.username.decode("utf-8")+".jpg"
    vf.save(path)
    print("need verify image, see: "+path)
    code = input("Input:")
    vf.input(code.encode("utf-8"))
    pass

def message_cb():
    for msg in lc.msg_list.read():
        print(msg.trycast(BuddyMessage))
        print(msg.typeid)
        if msg.trycast(Message):
            m = Message(msg.ref)
            print(m.sender)
            print(str(m))
        msg.destroy()
    prompt()

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
        print("not support js")
        exit(-1)
    else:
        js = lwjs.Lwjs()
        hashjs = urllib.request.urlopen("http://pidginlwqq.sinaapp.com/hash.js")
        js.load(hashjs.read())
        lc.get_friends_info(js.hashfunc,js.js).addListener(load_group_info)
    pass

def load_group_info():
    ev = Evset.new()
    lc.get_group_list().addto(ev)
    lc.get_discu_list().addto(ev)
    ev.addListener(poll_msg)

def poll_msg():
    lc.msg_list.poll(0)
    prompt()

def ignore():
    return

ls = ArgsParser(prog='ls',description='list friends',add_help=False)
ls.add_argument('who',help='friends uin',nargs='?')
ls.add_argument('-h',help='print help',action='store_true')

def list_friend(argv):
    args = ls.parse_args(argv)
    print(args)
    if args.h:
        ls.print_help()
        return
    if not args.who:
        for b in lc.friends():
            print('[Buddy uin:{} nick:{}]'.format(b.uin,b.nick.decode()))
        print()
        for g in lc.groups():
            print('[Group gid:{} name:{}]'.format(g.gid,g.name.decode()))
        print()
        for d in lc.discus():
            print('[Discu did:{} name:{}]'.format(d.did,d.name.decode()))
        return
    b = lc.find_buddy(uin=args.who.encode())
    g = lc.find_group(gid=args.who.encode())
    d = lc.find_discu(did=args.who.encode())
    if not b and not g and not d:
        print("not found")
        return
    found = b if b else g if g else d
    if not found.qqnumber:
        found.get_qqnumber()
        found.get_detail().addListener(
                functools.partial(list_friend,argv)
                )
        print("waiting for a while, fetch from server")
        return
    if b:
        c = b.get_category()
        if c: print('in Category:'+c.name.decode())
    print(str(b))

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

def talk_mode(fd,events):
    print()
    prompt("Input:")
    send = read(fd,512)[:-1]
    if send == "":
        loop.remove_handler(0)
        loop.add_handler(0,command,loop.READ)
    else:
        txt = Text.T(send)
        BuddyMessage().append(txt)

def talk_to(argv):
    args = talk.parse_args(argv)
    if args.to:
        b = lc.find_buddy(uin=args.to)
        if b:
            loop.remove_handler(0)
            loop.add_handler(0,talk_mode,loop.READ)


def command(fd,events):
    argv = read(fd,100).decode().rstrip().split(' ')
    if argv[0]=='ls' or argv[0] == 'l':
        list_friend(argv[1:])
    elif argv[0]=='quit' or argv[0]=='q':
        quit_program(argv[1:])
    print()
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


print(Lwqq.time())
Lwqq.log_level(3)
lc.setDispatcher(dispatch)
init_listener(lc)
loop.add_callback(main)
loop.add_handler(0,command,loop.READ)
loop.start()


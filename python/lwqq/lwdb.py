
# include from lwdb.h

import ctypes
from .base import lib, LwqqBase
from .lwqq import Buddy, Group, Lwqq
from .types import ErrorCode
from ctypes import POINTER

__all__ = ['Lwdb']

class Lwdb(LwqqBase):
    def __init__(self, lc, directory=None):
        self.ref = lib.lwdb_userdb_new(ctypes.c_char_p(lc.username), directory, 0)
    def __del__(self):
        lib.lwdb_userdb_free(self.ref)

    def begin(self):
        lib.lwdb_userdb_begin(self.ref)
    def commit(self):
        lib.lwdb_userdb_commit(self.ref)
    def insert(self, ins):
        if isinstance(ins, Buddy):
            return lib.lwdb_userdb_insert_buddy_info(self.ref, ctypes.byref(ins.ref))
        elif isinstance(ins, Group):
            return lib.lwdb_userdb_insert_group_info(self.ref, ctypes.byref(ins.ref))
    def update(self, ins):
        if isinstance(ins, Buddy):
            return lib.lwdb_userdb_update_buddy_info(self.ref, ctypes.byref(ins.ref))
        elif isinstance(ins, Group):
            return lib.lwdb_userdb_update_group_info(self.ref, ctypes.byref(ins.ref))
    def query(self, ins):
        if isinstance(ins, Buddy):
            return lib.lwdb_userdb_query_buddy(self.ref, ins.ref)
        elif isinstance(ins, Group):
            return lib.lwdb_userdb_query_group(self.ref, ins.ref)
    def query_qqnumbers(self, lc):
        lib.lwdb_userdb_query_qqnumbers(self.ref, lc.ref)


def register_library(lib):
    lib.lwdb_get_config_dir.restype = ctypes.c_char_p

    lib.lwdb_userdb_new.argtypes = [ctypes.c_char_p, ctypes.c_char_p,
            ctypes.c_int]
    lib.lwdb_userdb_new.restype = ctypes.c_voidp
    lib.lwdb_userdb_free.argtypes = [ctypes.c_voidp]

    lib.lwdb_userdb_insert_buddy_info.argtypes = [ctypes.c_voidp,
            POINTER(Buddy.PT)]
    lib.lwdb_userdb_insert_buddy_info.restype = ErrorCode

    lib.lwdb_userdb_insert_group_info.argtypes = [ctypes.c_voidp,
            POINTER(Group.PT)]
    lib.lwdb_userdb_insert_group_info.restype = ErrorCode

    lib.lwdb_userdb_update_buddy_info.argtypes = [ctypes.c_voidp,
            POINTER(Buddy.PT)]
    lib.lwdb_userdb_update_buddy_info.restype = ErrorCode

    lib.lwdb_userdb_update_group_info.argtypes = [ctypes.c_voidp,
            POINTER(Group.PT)]
    lib.lwdb_userdb_update_group_info.restype = ErrorCode

    lib.lwdb_userdb_flush_buddies.argtypes = [ctypes.c_voidp, ctypes.c_int,
            ctypes.c_int]
    lib.lwdb_userdb_flush_groups.argtypes = [ctypes.c_voidp, ctypes.c_int,
            ctypes.c_int]
    lib.lwdb_userdb_query_qqnumbers.artypes = [ctypes.c_voidp, Lwqq.PT]
    lib.lwdb_userdb_query_buddy.argtypes = [ctypes.c_voidp, Buddy.PT]
    lib.lwdb_userdb_query_buddy.restype = ErrorCode
    lib.lwdb_userdb_query_group.argtypes = [ctypes.c_voidp, Group.PT]
    lib.lwdb_userdb_query_group.restype = ErrorCode

    lib.lwdb_userdb_begin.argtype = [ctypes.c_voidp]
    lib.lwdb_userdb_commit.argtype = [ctypes.c_voidp]

    lib.lwdb_userdb_read.argtypes = [ctypes.c_voidp, ctypes.c_char_p]
    lib.lwdb_userdb_read.restype = ctypes.c_char_p
    lib.lwdb_userdb_write.argtypes = [ctypes.c_voidp, ctypes.c_char_p,
            ctypes.c_char_p]
    lib.lwdb_userdb_write.restype = ctypes.c_int

register_library(lib)



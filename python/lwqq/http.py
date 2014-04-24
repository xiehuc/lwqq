
# include from http.h

from .base import lib

from ctypes import c_long,c_char_p,c_int,c_voidp,c_size_t
from ctypes import Structure

class HttpHandle(Structure):
    class Proxy(Structure):
        _fields_ = [
                ('type',c_int),
                ('host',c_char_p),
                ('port',c_int),
                ('username',c_char_p),
                ('password',c_char_p)
                ]
    _fields_ = [
            ('proxy',Proxy),
            ('quit',c_int),
            ('synced',c_int)
            ]

# vim: ts=3 sts=3 sw=3 et

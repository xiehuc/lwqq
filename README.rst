======
lwqq
======

Introduction
==============


updating here:

This is zip, Im just forking this project for personal use and other guys who's still using old linux box.

Im using Linux Fedora:: 
    
    kernel-2.6.43.8-1.fc15.i686
    with:
    gcc-4.6.3-2.fc15.i686
    cmake-2.8.4-1.fc15.i686
    make-3.82-4.fc15.i686
    
    ev lib:
    libev-4.03-2.fc15.i686
    libev-devel-4.03-2.fc15.i686
    
    mozjs libs:
    js-devel-1.8.5-6.fc15.i686


orig. update cannot work now on my box, with ERROR mesg:

"""

CMake Error at lib/CMakeLists.txt:79 (GENERATE_EXPORT_HEADER):
  Unknown CMake command "GENERATE_EXPORT_HEADER".

"""

it's involved by::
    
    commit 0af7cac3e3845d325a8a1e5a01476022dcd59f4d
    " add LWQQ_EXPORT for win dll export "

no use here for my using linux only, I dont have to upgrade my cmake utils.


for fun.


==============


Following doc copied from xiehuc/lwqq:

lwqq means light weight QQ (also Linux webqq). It provide a library for the webqq
protocol.

lwqq is designed as cross platform and also provide a Python API to support quick
development.

lwqq is currently used in many open source projects. The most well known is `pidgin-lwqq`__

__ https://github.com/xiehuc/pidgin-lwqq

lwqq is licenced under the **GPLv3** License.


Quick Build
=============

Most of lwqq's dependencies are optional. It means that you can write some own implementations
and embed it into a small applications.

A brief install guide::
   
   mkdir build;cd build
   cmake ..
   make 
   sudo make install

You can fin more details concerning the build process in the wiki__

__ https://github.com/xiehuc/lwqq/wiki/Build-From-Source

Debian Package
=============

To create a Debian package from the source::

   mkdir build;cd build
   cmake ..
   cpack


======
lwqq
======

Introduction
==============

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
   make python  # create python egg install package
   easy_install python/dist/*.egg # install python bindings (optional)

You can fin more details concerning the build process in the wiki__

__ https://github.com/xiehuc/lwqq/wiki/Build-From-Source

Debian Package
=============

To create a Debian package from the source::

   mkdir build;cd build
   cmake ..
   cpack


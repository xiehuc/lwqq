# vim: ts=3 sts=3 sw=3 noet

project(liblwqq)
cmake_minimum_required(VERSION 2.8)
set(VERSION_MAJOR "0")
set(VERSION_MINOR "4")
set(VERSION_PATCH "0")

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

add_definitions(-Wall)
option(WITHOUT_ASYNC "Fully Disable Async Ability, Only Sync mode" Off)
set(QUICK_QUIT False CACHE BOOL "Hide Option: Detatch thread to quickly quit")

find_package(Sphinx)
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	find_package(PkgConfig REQUIRED)
	#recommand libcurl>=7.22.0
	pkg_check_modules(CURL REQUIRED libcurl)
	pkg_check_modules(SQLITE REQUIRED sqlite3)
	pkg_check_modules(ZLIB REQUIRED zlib)
	pkg_check_modules(MOZJS mozjs-17.0)
	set(MOZJS_17 ${MOZJS_FOUND} CACHE BOOL "use mozjs-17.0 version")
	if(NOT MOZJS_FOUND)
		pkg_check_modules(MOZJS mozjs185)
		set(MOZJS_185 ${MOZJS_FOUND} CACHE BOOL "use mozjs185 version")
	endif()
	find_package(EV)
	find_package(Libuv)
endif()

if(CYGWIN)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(CURL REQUIRED libcurl)
	pkg_check_modules(SQLITE REQUIRED sqlite3)
	pkg_check_modules(ZLIB REQUIRED zlib)
	pkg_check_modules(MOZJS mozjs-17.0)
	set(MOZJS_17 ${MOZJS_FOUND} CACHE BOOL "use mozjs-17.0 version")
	if(NOT MOZJS_FOUND)
		pkg_check_modules(MOZJS mozjs185)
		set(MOZJS_185 ${MOZJS_FOUND} CACHE BOOL "use mozjs185 version")
	endif()
	find_package(EV)
	#recommand libcurl>=7.22.0
	if(WITH_LIBUV)
		find_package(Libuv REQUIRED)
	endif()
endif()

if(APPLE)
	find_package(PkgConfig REQUIRED)
	find_package(CURL REQUIRED)
	find_package(Sqlite3 REQUIRED)
	find_package(ZLIB REQUIRED)
	pkg_check_modules(MOZJS mozjs-17.0)
	set(MOZJS_17 ${MOZJS_FOUND} CACHE BOOL "use mozjs-17.0 version")
	if(NOT MOZJS_FOUND)
		pkg_check_modules(MOZJS mozjs185)
		set(MOZJS_185 ${MOZJS_FOUND} CACHE BOOL "use mozjs185 version")
	endif()
	find_package(EV)
	find_package(Libuv)
endif()

if(WIN32)
	set(CMAKE_PREFIX_PATH 
		"${WIN32_LIBS_DIR}/libcurl" 
		"${WIN32_LIBS_DIR}/libcurl/include" 
		"${WIN32_LIBS_DIR}/libcurl/lib"
		)
	find_package(CURL REQUIRED)
	set(CMAKE_PREFIX_PATH "${WIN32_LIBS_DIR}/sqlite3")
	find_package(Sqlite3 REQUIRED)
	set(CMAKE_PREFIX_PATH 
		"${WIN32_LIBS_DIR}/zlib/include" 
		"${WIN32_LIBS_DIR}/zlib/lib"
		)
	find_package(ZLIB REQUIRED)
	set(CMAKE_PREFIX_PATH 
		"${WIN32_LIBS_DIR}/libuv/include" 
		"${WIN32_LIBS_DIR}/libuv/Release" 
		"${WIN32_LIBS_DIR}/libuv/Debug")
	find_package(Libuv)
	set(CMAKE_PREFIX_PATH 
		"${WIN32_LIBS_DIR}/js-1.8.5-mingw32/include/js" 
		"${WIN32_LIBS_DIR}/js-1.8.5-mingw32/lib"
		)
	find_package(MozJS REQUIRED)
	set(MOZJS_185 ${MOZJS_FOUND})
	find_package(EV)

	include(GenerateExportHeader)
endif(WIN32)

include(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(strtok_r HAVE_STRTOK_R)

option(WITH_LIBEV "Use Libev To Provide Async " ${EV_FOUND})
if(WITH_LIBEV AND NOT EV_FOUND)
	message(FATAL_ERROR "Required package libev not found")
endif()
if(WIN32)
	#only windows open libuv by default
	option(WITH_LIBUV "Use Libuv To Provide Async " ${UV_FOUND})
else()
	option(WITH_LIBUV "Use Libuv To Provide Async " OFF)
endif()
if(WITH_LIBUV AND NOT UV_FOUND)
	message(FATAL_ERROR "Required package libuv not found")
endif()

if(WITH_LIBEV AND WITH_LIBUV)
	message(FATAL_ERROR "Libuv and Libev shouldn't used together; Turn off one")
endif()

option(WITH_MOZJS "Use Mozjs To Provide hash.js support" ${MOZJS_FOUND})
if(WITH_MOZJS AND NOT MOZJS_FOUND)
	message(FATAL_ERROR "Reuired mozjs185 or mozjs-17.0 package not found")
endif()

option(ENABLE_DOCS "Build Documentation" OFF)
if(ENABLE_DOCS AND NOT SPHINX_FOUND)
	message(FATAL_ERROR "Reuired python-sphinx package not found")
endif()

#always true because we force enable sqlite
set(WITH_SQLITE True)

set(LIB_INSTALL_DIR "lib" CACHE STRING "the directory to install library")

message( "================lwqq flags=====================")
if(WIN32)
	message(STATUS "With Libuv (Option)     : ${WITH_LIBUV}")
else()
	message(STATUS "With Libev (Option)     : ${WITH_LIBEV}")
endif()
message(STATUS "With Mozjs (Option)     : ${WITH_MOZJS}")
message(STATUS "Build Document (Option) : ${ENABLE_DOCS}")
message( "===============================================")

set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

string(COMPARE EQUAL "${CMAKE_BUILD_TYPE}" "Debug" USE_DEBUG)
configure_file(config.h.in ${PROJECT_BINARY_DIR}/lwqq-config.h)
configure_file(lwqq.pc.in ${PROJECT_BINARY_DIR}/lwqq.pc @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/lwqq-config.h" DESTINATION include/lwqq)
install(FILES "${PROJECT_BINARY_DIR}/lwqq.pc" DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(python)
if(ENABLE_DOCS)
	add_subdirectory(doc)
endif()

# package settings
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "lwqq provide a lightweight interconnection library to connect to the QQ network threw WebQQ")
set(CPACK_PACKAGE_VENDOR "xiehuc")
set(CPACK_PACKAGE_DESCRIPTION "WebQQ protocol interconnection protocol (library)")
set(CPACK_PACKAGE_CONTACT "edhelas@movim.eu")
set(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON) 
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${VERSION}")
set(ACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}_${VERSION}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jaussoin Timothée") #required

set(PACK "DEB" CACHE STRING "Generate a Package")
set(CPACK_GENERATOR ${PACK})

include(CPack)

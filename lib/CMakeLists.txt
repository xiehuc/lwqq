set(LWQQ_LIST
    url.c
    md5.c
    logger.c
    login.c
    json.c
    msg.c
    type.c
    smemory.c
    info.c
    swsqlite.c
    vplist.c
    utility.c
    internal.c
    async.c
    http.c
    lwdb.c
	 lwjs.c
    )
set(LWQQ_HEADER
	 queue.h
    async.h
    async_impl.h
    http.h
    info.h
    logger.h
    login.h
    lwdb.h
    msg.h
    smemory.h
    type.h
    utility.h
    lwqq.h
    vplist.h
    swsqlite.h
    lwjs.h
    )
add_definitions(-Wall )

include_directories(
    ${ZLIB_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${SQLITE_INCLUDE_DIRS}
    ${MOZJS_INCLUDE_DIRS}
    ${PROJECT_BINARY_DIR}
    )

if(UNIX)
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-strict-aliasing")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ")
endif(UNIX)

if(WIN32)
  set(LWQQ_LIST ${LWQQ_LIST} win32.c )
  add_definitions(-D_GNU_SOURCE=1)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static -static-libgcc -static-libstdc++" )
  set(LWQQ_OTHER_LIBS 
     ${LWQQ_OTHER_LIBS} 
     ws2_32
     "-Wl,-Bdynamic -lpthread"
     )
endif(WIN32)

if(WITH_LIBEV)
    include_directories(${EV_INCLUDE_DIRS})
    set(LWQQ_OTHER_LIBS ${EV_LIBRARIES} ${LWQQ_OTHER_LIBS})
endif()
if(WITH_LIBUV)
    include_directories(${UV_INCLUDE_DIRS})
    set(LWQQ_OTHER_LIBS ${LWQQ_OTHER_LIBS} ${UV_LIBRARIES})
endif()

add_library(lwqq-static STATIC ${LWQQ_LIST} )
set_target_properties(lwqq-static PROPERTIES 
    OUTPUT_NAME lwqq
    COMPILE_FLAGS -DLWQQ_STATIC_DEFINE
    )
add_library(lwqq SHARED ${LWQQ_LIST} )
GENERATE_EXPORT_HEADER(lwqq 
    EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/lwqq_export.h
    )

target_link_libraries(lwqq
    ${SQLITE_LIBRARIES}
    ${CURL_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${MOZJS_LIBRARIES}
    ${LWQQ_OTHER_LIBS}
    )
set_target_properties(lwqq PROPERTIES
   VERSION "${VERSION}"
   SOVERSION "${VERSION_MAJOR}"
   )

target_link_libraries(lwqq-static
    ${SQLITE_LIBRARIES}
    ${CURL_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${MOZJS_LIBRARIES}
    ${LWQQ_OTHER_LIBS}
    )


install(TARGETS lwqq DESTINATION ${LIB_INSTALL_DIR})
install(TARGETS lwqq-static DESTINATION ${LIB_INSTALL_DIR})
install(FILES ${LWQQ_HEADER} DESTINATION include)
message("NOTE!! add 'export LD_LIBRARY_PATH=${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}:$LD_LIBRARY_PATH' "
	"to ~/.xprofile when need load library from desktop application such as pidgin")

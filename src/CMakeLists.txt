add_executable(lwqq-cli cli.c)

include_directories(
    ${PROJECT_BINARY_DIR}
	../lib 
	)

if(WIN32)
	target_link_libraries(lwqq-cli lwqq-static)
else()
	target_link_libraries(lwqq-cli lwqq)
endif()

# after 0.4.0 use lwqq-cli.py instead
# install(TARGETS lwqq-cli DESTINATION bin)

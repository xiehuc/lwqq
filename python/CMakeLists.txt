set(output "${PROJECT_BINARY_DIR}/setup.py")
configure_file(setup.py.in ${output} @ONLY)

add_custom_target(python
   python ${output} bdist_egg
   SOURCES ${output}
   )

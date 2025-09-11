set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../frontend")
if(NOT EXISTS ${WEB_SRC_DIR}/dist)
    message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exit. Please run 'npm run build' in ${WEB_SRC_DIR}")
endif()

file(GLOB_RECURSE WEB_SRCS ${WEB_SRC_DIR}/dist/*/* ${WEB_SRC_DIR}/dist/*)

idf_component_register(SRCS "www_bin.c"
                       INCLUDE_DIRS "include"
                       EMBED_TXTFILES ${WEB_SRCS})

idf_build_get_property(python PYTHON)

###################
# Generate files for www_bin
add_custom_target(OUTPUT "www_bin.c"
                  COMMAND ${python} "${CMAKE_CURRENT_SOURCE_DIR}/generate/generate.py" "${WEB_SRC_DIR}/dist"
                  COMMENT "-- Generating www_bin sources"
                  VERBATIM)

set_property(DIRECTORY "${COMPONENT_PATH}" APPEND PROPERTY
             ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_SOURCE_DIR}/www_bin.c
                                         ${CMAKE_CURRENT_SOURCE_DIR}/include/www_bin.h)

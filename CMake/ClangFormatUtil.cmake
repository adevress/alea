# ClangFormatUtil.cmake
#
#
# Clang format convenience wrapper
# make clang format to reformat all cpp code by a simple 'make clang-format"
#
# License: Boost Software License 1.0
#
# Copyright (c) 2021, Adrien Devresse
#
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          https://www.boost.org/LICENSE_1_0.txt)


function(define_clang_format_target)
    if(NOT ${PROJECT_NAME}_CLANG_FORMAT_BINARY)
        find_program(${PROJECT_NAME}_CLANG_FORMAT_BINARY clang-format)
    endif()


    if(${PROJECT_NAME}_CLANG_FORMAT_BINARY)
        get_property(CLANG_FORMAT_ALL_SRC GLOBAL PROPERTY format_source_list_property)
        add_custom_target(format
            COMMAND ${${PROJECT_NAME}_CLANG_FORMAT_BINARY}
            -i ${CLANG_FORMAT_ALL_SRC})

        add_custom_target(format-check
            COMMAND ${${PROJECT_NAME}_CLANG_FORMAT_BINARY}
	    --Werror --dry-run  -i ${CLANG_FORMAT_ALL_SRC})
    endif()
endfunction()


function(add_target_source_for_format target1)
    get_target_property(TARGET_SRC_FILES ${target1} SOURCES)

    get_property(CLANG_FORMAT_ALL_SRC GLOBAL PROPERTY format_source_list_property)
    list(APPEND CLANG_FORMAT_ALL_SRC ${TARGET_SRC_FILES})
    set_property(GLOBAL PROPERTY format_source_list_property "${CLANG_FORMAT_ALL_SRC}")
endfunction()

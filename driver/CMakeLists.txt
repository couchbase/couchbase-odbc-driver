function (declare_odbc_lib_targets libname UNICODE)

# See the usage of each workaround in the code, for comments.
unset (WORKAROUND_ALLOW_UNSAFE_DISPATCH)
unset (WORKAROUND_USE_ICU)
unset (WORKAROUND_ENABLE_TRIM_TRAILING_NULL)
unset (WORKAROUND_ENABLE_DEFINE_SQLBindParam)
unset (WORKAROUND_ENABLE_NO_UNICODE_CHARS_IN_COLUMN_NAMES_IN_TESTS)

if (UNICODE)
    set (DRIVER_PREFIX Unicode)
    set (WIDECHAR_SUFFIX w)
else ()
    set (DRIVER_PREFIX ANSI)
    set (WIDECHAR_SUFFIX)
    if ("${ODBC_PROVIDER}" STREQUAL "UnixODBC")
        set (WORKAROUND_ENABLE_TRIM_TRAILING_NULL 1)
    endif ()
endif ()

if ("${ODBC_PROVIDER}" STREQUAL "iODBC")
#   set (WORKAROUND_ENABLE_DEFINE_SQLBindParam 1)
    set (WORKAROUND_ENABLE_NO_UNICODE_CHARS_IN_COLUMN_NAMES_IN_TESTS 1)
endif ()

if (CH_ODBC_ALLOW_UNSAFE_DISPATCH)
    set (WORKAROUND_ALLOW_UNSAFE_DISPATCH 1)
endif()

if (CH_ODBC_USE_ICU)
    set (WORKAROUND_USE_ICU 1)
endif ()

message (STATUS "${libname}: ${DRIVER_PREFIX}")

configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/config_cmake.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/platform/config_cmake${WIDECHAR_SUFFIX}.h
)

unset (WORKAROUND_ALLOW_UNSAFE_DISPATCH)
unset (WORKAROUND_USE_ICU)
unset (WORKAROUND_ENABLE_TRIM_TRAILING_NULL)
unset (WORKAROUND_ENABLE_DEFINE_SQLBindParam)
unset (WORKAROUND_ENABLE_NO_UNICODE_CHARS_IN_COLUMN_NAMES_IN_TESTS)

# In order to enable testing, put every non-public symbol to a static library (which is then used by shared library and unit-test binary).
add_library (${libname}-impl STATIC
    utils/source/type_parser.cpp
    utils/source/type_info.cpp
    utils/source/unicode_converter.cpp
    utils/source/conversion_context.cpp
    utils/source/ieee_754_converter.cpp
    utils/source/lossless-adm_to_human-readable_format.cpp
    utils/source/WinReg_Value_Fetcher.cpp

    config/config.cpp

    escaping/source/escape_sequences.cpp
    escaping/source/lexer.cpp

    format/source/ODBCDriver2.cpp
    format/source/CBAS.cpp
    format/source/RowBinaryWithNamesAndTypes.cpp

    api/impl/impl.cpp

    source/attributes.cpp
    source/connection.cpp
    source/cJSON.c
    source/descriptor.cpp
    source/diagnostics.cpp
    source/driver.cpp
    source/environment.cpp
    source/exception.cpp
    source/object.cpp
    source/result_set.cpp
    source/statement.cpp


    ${CMAKE_CURRENT_BINARY_DIR}/platform/config_cmake${WIDECHAR_SUFFIX}.h
    platform/platform.h

    utils/include/utils.h
    utils/include/iostream_debug_helpers.h
    utils/include/amortized_istream_reader.h
    utils/include/resize_without_initialization.h
    utils/include/object_pool.h
    utils/include/string_pool.h
    utils/include/unicode_converter.h
    utils/include/conversion_context.h
    utils/include/conversion.h
    utils/include/conversion_std.h
    utils/include/conversion_icu.h
    utils/include/type_parser.h
    utils/include/type_info.h
    utils/include/ieee_754_converter.h

    config/config.h
    config/ini_defines.h

    escaping/include/escape_sequences.h
    escaping/include/lexer.h
    escaping/include/lexer_declare.h
    escaping/include/function_declare.h

    api/impl/impl.h

    format/include/ODBCDriver2.h
    format/include/CBAS.h
    format/include/RowBinaryWithNamesAndTypes.h

    include/attributes.h
    include/connection.h
    include/cJSON.h
    include/descriptor.h
    include/diagnostics.h
    include/driver.h
    include/environment.h
    include/exception.h
    include/object.h
    include/result_set.h
    include/statement.h
)

set (WIN_SOURCES)
if (WIN32)
    set (WIN_SOURCES
        platform/win/dllmain.cpp
        platform/win/resource.h
        platform/win/setup.cpp
        platform/win/resource.rc
    )

    if (NOT "${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
        # An entry in .def file still must be added for each exported function for 32-bit targets.
        # See also the definition of 'EXPORTED_FUNCTION' C/C++ macro in the code.
        set (WIN_SOURCES
            ${WIN_SOURCES}
            platform/win/${libname}.def
        )
    endif ()
endif ()

if (UNICODE)
    target_compile_definitions (${libname}-impl
        PUBLIC UNICODE
        PUBLIC _UNICODE
    )
endif ()

target_include_directories (${libname}-impl
    PUBLIC ${PROJECT_BINARY_DIR}
    PUBLIC ${PROJECT_SOURCE_DIR}
    PUBLIC ${PROJECT_SOURCE_DIR}/contrib/libcouchbase/include
    PUBLIC ${PROJECT_BINARY_DIR}/contrib/libcouchbase/generated
)

set (_poco_net Poco::Net)

target_link_libraries (${libname}-impl
    PUBLIC ${_poco_net}
    PUBLIC Poco::Foundation
    PUBLIC ODBC::Driver
    PUBLIC Threads::Threads
    PUBLIC couchbase
)

if (CH_ODBC_USE_ICU)
    target_link_libraries (${libname}-impl
        PUBLIC ICU::uc
    )
endif ()

add_library (${libname} SHARED
    api/odbc.cpp
    ${WIN_SOURCES}
)

set_target_properties (${libname} PROPERTIES
    OUTPUT_NAME "couchbaseodbc${WIDECHAR_SUFFIX}"
)

target_link_libraries (${libname}
    PRIVATE ${libname}-impl
)

if (NOT APPLE AND NOT WIN32)
    target_link_options (${libname}
        PRIVATE "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/platform/version_script"
    )
endif ()

if (MSVC)
    install (
        TARGETS ${libname}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT "${DRIVER_PREFIX}Driver"
    )
    install (
        FILES $<TARGET_PDB_FILE:${libname}>
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT "${DRIVER_PREFIX}DriverDebugSymbols"
        OPTIONAL
    )
else ()
    install (
        TARGETS ${libname}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT "${DRIVER_PREFIX}Driver"
    )
endif ()

endfunction ()

declare_odbc_lib_targets (couchbase-odbc 0)
declare_odbc_lib_targets (couchbase-odbcw 1)
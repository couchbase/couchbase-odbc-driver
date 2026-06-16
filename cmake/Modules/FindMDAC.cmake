#
# Consults (some of) the following vars (if set):
#     ODBC_MDAC_DIR
#
# Defines (some of) the following vars:
#     MDAC_FOUND
#     ODBC_MDAC_FOUND
#
#     ODBC_MDAC_APP_DEFINES
#     ODBC_MDAC_APP_INCLUDE_DIRS
#     ODBC_MDAC_APP_COMPILER_FLAGS
#     ODBC_MDAC_APP_LINKER_FLAGS
#
#     ODBC_MDAC_DRIVER_DEFINES
#     ODBC_MDAC_DRIVER_INCLUDE_DIRS
#     ODBC_MDAC_DRIVER_COMPILER_FLAGS
#     ODBC_MDAC_DRIVER_LINKER_FLAGS
#

set (ODBC_MDAC_FOUND TRUE)

# CMake 4.0+ no longer searches INCLUDE/LIB env vars in find_path/find_library.
# Derive Windows SDK paths from CMAKE_MT which the Visual Studio generator sets
# (e.g. C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/mt.exe).
set (_winsdk_include_hint)
set (_winsdk_lib_hint)
set (_msvc_lib_hint)
if (CMAKE_MT)
    string (TOLOWER "${CMAKE_MT}" _cmake_mt_lc)
    string (REGEX REPLACE "/bin/[^/]+/[^/]+/mt\\.exe$" "" _winsdk_root "${_cmake_mt_lc}")
    string (REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+" _winsdk_version "${_cmake_mt_lc}")
    if (_winsdk_root AND _winsdk_version)
        set (_winsdk_include_hint "${_winsdk_root}/Include/${_winsdk_version}/um")
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set (_winsdk_lib_hint "${_winsdk_root}/Lib/${_winsdk_version}/um/x64")
        else ()
            set (_winsdk_lib_hint "${_winsdk_root}/Lib/${_winsdk_version}/um/x86")
        endif ()
    endif ()
endif ()
# legacy_stdio_definitions lives in the MSVC toolchain lib dir, not the Windows SDK.
# Derive it from CMAKE_LINKER: .../MSVC/<ver>/bin/Host<arch>/<arch>/link.exe -> .../MSVC/<ver>/lib/<arch>
if (CMAKE_LINKER)
    string (TOLOWER "${CMAKE_LINKER}" _cmake_linker_lc)
    string (REGEX REPLACE "/bin/host[^/]+/[^/]+/link\\.exe$" "" _msvc_root "${_cmake_linker_lc}")
    if (_msvc_root)
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set (_msvc_lib_hint "${_msvc_root}/lib/x64")
        else ()
            set (_msvc_lib_hint "${_msvc_root}/lib/x86")
        endif ()
    endif ()
endif ()

foreach (_role_lc app driver)
    if (NOT WIN32)
        set (ODBC_MDAC_FOUND FALSE)
        message(WARNING "ODBC MDAC: Not available on non-Windows systems.")
        break ()
    endif ()

    set (_headers sql.h;sqltypes.h;sqlucode.h;sqlext.h)
    set (_libs)

    if ("${_role_lc}" STREQUAL "app")
        list (APPEND _libs odbc32)
    elseif ("${_role_lc}" STREQUAL "driver")
        list (APPEND _headers odbcinst.h)
        list (APPEND _libs odbccp32)

        if (MSVC OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")
            list (APPEND _libs ws2_32)
        endif ()

        # Starting from Visual Studio 2015, odbccp32 depends on symbols
        # which are moved to legacy_stdio_definitions lib.
        if (MSVC_TOOLSET_VERSION GREATER_EQUAL 140)
            list (APPEND _libs legacy_stdio_definitions)
        endif ()
    endif ()

    set(_found_${_role_lc}_include_dirs)
    foreach (_file ${_headers})
        unset (_path CACHE)
        unset (_path)

        if (ODBC_MDAC_DIR)
            find_path (_path
                NAMES "${_file}"
                PATHS "${ODBC_MDAC_DIR}"
                NO_DEFAULT_PATH
            )
        else ()
            find_path (_path
                NAMES "${_file}"
                HINTS "${_winsdk_include_hint}"
            )
        endif ()

        if (_path)
            list (APPEND _found_${_role_lc}_include_dirs "${_path}")
        else ()
            set (ODBC_MDAC_FOUND FALSE)
            message(WARNING "ODBC MDAC: Failed to locate path containing '${_file}' header file.")
        endif ()
    endforeach ()
    list (REMOVE_DUPLICATES _found_${_role_lc}_include_dirs)

    set(_found_${_role_lc}_linker_flags)
    foreach (_file ${_libs})
        unset (_path CACHE)
        unset (_path)

        if (ODBC_MDAC_DIR)
            find_library (_path
                NAMES "${_file}"
                PATHS "${ODBC_MDAC_DIR}"
                NO_DEFAULT_PATH
            )
        else ()
            find_library (_path
                NAMES "${_file}"
                HINTS "${_winsdk_lib_hint}" "${_msvc_lib_hint}"
            )
        endif ()

        if (_path)
            list (APPEND _found_${_role_lc}_linker_flags "${_path}")
        else ()
            set (ODBC_MDAC_FOUND FALSE)
            message(WARNING "ODBC MDAC: Failed to locate '${_file}' library file.")
        endif ()
    endforeach ()
    list (REMOVE_DUPLICATES _found_${_role_lc}_linker_flags)
endforeach ()

unset (_path CACHE)
unset (_path)
unset (_file)
unset (_libs)
unset (_headers)
unset (_cmake_mt_lc)
unset (_cmake_linker_lc)
unset (_winsdk_include_hint)
unset (_winsdk_lib_hint)
unset (_winsdk_root)
unset (_winsdk_version)
unset (_msvc_lib_hint)
unset (_msvc_root)

if (ODBC_MDAC_FOUND)
    foreach (_role_uc APP DRIVER)
        string (TOLOWER "${_role_uc}" _role_lc)
        foreach (_comp_uc DEFINES INCLUDE_DIRS COMPILER_FLAGS LINKER_FLAGS)
            string (TOLOWER "${_comp_uc}" _comp_lc)
            set (ODBC_MDAC_${_role_uc}_${_comp_uc})
            if (_found_${_role_lc}_${_comp_lc})
                set (ODBC_MDAC_${_role_uc}_${_comp_uc} ${_found_${_role_lc}_${_comp_lc}})
            endif ()
            mark_as_advanced (ODBC_MDAC_${_role_uc}_${_comp_uc})
        endforeach ()
    endforeach ()
endif ()

foreach (_role_lc app driver)
    foreach (_comp_lc defines include_dirs compiler_flags linker_flags)
        unset (_found_${_role_lc}_${_comp_lc})
    endforeach ()
endforeach ()

unset (_comp_lc)
unset (_comp_uc)
unset (_role_lc)
unset (_role_uc)

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (MDAC REQUIRED_VARS ODBC_MDAC_FOUND)

set (ODBC_MDAC "${MDAC_FOUND}")

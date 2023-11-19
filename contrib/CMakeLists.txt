get_filename_component (_current_dir_name "${CMAKE_CURRENT_LIST_DIR}" NAME)
if (CMAKE_FOLDER)
    set (CMAKE_FOLDER "$CMAKE_FOLDER/${_current_dir_name}")
else ()
    set (CMAKE_FOLDER "${_current_dir_name}")
endif ()
unset (_current_dir_name)

set(LCB_NO_TESTS ON CACHE BOOL "" FORCE)
set(LCB_NO_TOOLS ON CACHE BOOL "" FORCE)
set(LCB_BUILD_DTRACE OFF CACHE BOOL "" FORCE)
set(LCB_UNINSTALL_TARGET OFF CACHE BOOL "" FORCE)
set(LCB_BUILD_STATIC 1 CACHE INTERNAL "" FORCE)
add_subdirectory (libcouchbase)

if (CH_ODBC_PREFER_BUNDLED_POCO OR NOT Poco_FOUND)
    if (NOT Poco_FOUND AND NOT CH_ODBC_PREFER_BUNDLED_POCO)
        message (WARNING "Poco: unable to find system Poco, falling back to using the bundled variant of the library")
    endif ()

    if (CH_ODBC_THIRD_PARTY_LINK_STATIC)
        set (POCO_STATIC ON CACHE INTERNAL "")
    endif ()

    if (CH_ODBC_RUNTIME_LINK_STATIC)
        set (POCO_MT ON CACHE INTERNAL "")
    endif ()

    set (CMAKE_MODULE_PATH_bak ${CMAKE_MODULE_PATH})
    set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH_orig})
    #Disable unused sub-packages of poco
    set (ENABLE_ENCODINGS OFF CACHE BOOL "" FORCE)
    set (ENABLE_XML OFF CACHE BOOL "" FORCE)
    set (ENABLE_JSON OFF CACHE BOOL "" FORCE)
    set (ENABLE_MONGODB OFF CACHE BOOL "" FORCE)
    set (ENABLE_DATA_SQLITE OFF CACHE BOOL "" FORCE)
    set (ENABLE_REDIS OFF CACHE BOOL "" FORCE)
    set (ENABLE_PROMETHEUS OFF CACHE BOOL "" FORCE)
    set (ENABLE_UTIL OFF CACHE BOOL "" FORCE)
    set (ENABLE_ZIP OFF CACHE BOOL "" FORCE)
    set (ENABLE_PAGECOMPILER OFF CACHE BOOL "" FORCE)
    set (ENABLE_PAGECOMPILER_FILE2PAGE OFF CACHE BOOL "" FORCE)
    set (ENABLE_ACTIVERECORD OFF CACHE BOOL "" FORCE)
    set (ENABLE_ACTIVERECORD_COMPILER OFF CACHE BOOL "" FORCE)
    set (ENABLE_JWT OFF CACHE BOOL "" FORCE)
    set (ENABLE_CRYPTO OFF CACHE BOOL "" FORCE)
    set (ENABLE_DATA OFF CACHE BOOL "" FORCE)
    set (ENABLE_NETSSL OFF CACHE BOOL "" FORCE)

    add_subdirectory (poco)

    set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH_bak})
    unset (CMAKE_MODULE_PATH_bak)

    add_library(Poco::Net ALIAS Net)
    add_library(Poco::Foundation ALIAS Foundation)

    # Remove pthread from linking poco's Net
    set_property (TARGET Net PROPERTY INTERFACE_LINK_LIBRARIES "")
    set_property (TARGET Foundation PROPERTY INTERFACE_LINK_LIBRARIES "")
endif ()

if (MSVC)
    if (TARGET Foundation)
        set_property(TARGET Foundation APPEND PROPERTY INTERFACE_LINK_LIBRARIES Iphlpapi)
    elseif (TARGET Poco::Foundation)
        set_property(TARGET Poco::Foundation APPEND PROPERTY INTERFACE_LINK_LIBRARIES Iphlpapi)
    endif ()
endif ()

# Some of the above may define this target.
if (TARGET uninstall AND CMAKE_FOLDER)
    set_target_properties (uninstall PROPERTIES FOLDER "${CMAKE_FOLDER}")
endif ()

# Some of the above may set a cache entry that will interfere with later reconfigurations.
unset (ODBC_CONFIG CACHE)

cmake_minimum_required (VERSION 3.24.0)

set (CMAKE_MODULE_PATH_orig ${CMAKE_MODULE_PATH})
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")
include (cmake/prevent_in_source_builds.cmake)

project (
    couchbase-odbc
    VERSION 1.0.0 
    DESCRIPTION "The official ODBC driver implementation for accessing couchbase as a data source."
    HOMEPAGE_URL "https://github.com/couchbaselabs/couchbase-odbc-driver.git"
    LANGUAGES C CXX
)

set (CMAKE_C_STANDARD 11)
#set (CMAKE_C_EXTENSIONS 0)
set (CMAKE_C_STANDARD_REQUIRED 1)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_EXTENSIONS 0)
set (CMAKE_CXX_STANDARD_REQUIRED 1)

set (CMAKE_POSITION_INDEPENDENT_CODE 1)

set_property (GLOBAL PROPERTY USE_FOLDERS 1)
set (CMAKE_BUILD_COLOR_MAKEFILE 1)
set (CMAKE_INSTALL_DEFAULT_COMPONENT_NAME ANSIDriver)

if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "RelWithDebInfo")
endif ()

if (NOT CMAKE_CONFIGURATION_TYPES)
    set (CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}")
endif ()

message (STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

string (TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UC)
string (TOUPPER "${CMAKE_CONFIGURATION_TYPES}" CMAKE_CONFIGURATION_TYPES_UC)

include(CMakeDependentOption)

option(BUILD_TESTING "Build test targets" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

option (CH_ODBC_ALLOW_UNSAFE_DISPATCH "Allow unchecked handle dispatching (may slightly increase performance in some scenarios)" ON)
option (CH_ODBC_ENABLE_SSL "Enable SSL (required for utilizing https:// interface, etc.)" ON)
cmake_dependent_option (CH_ODBC_USE_ICU "Use ICU library, instead of C++ STD, for Unicode conversions" ON "NOT MSVC" OFF)
option (CH_ODBC_PREFER_BUNDLED_THIRD_PARTIES "Prefer bundled over system variants of third party libraries" ON)
cmake_dependent_option (CH_ODBC_PREFER_BUNDLED_POCO "Prefer bundled over system variants of Poco library" ON "CH_ODBC_PREFER_BUNDLED_THIRD_PARTIES" OFF)
cmake_dependent_option (CH_ODBC_PREFER_BUNDLED_SSL "Prefer bundled over system variants of SSL library" ON "CH_ODBC_PREFER_BUNDLED_POCO" OFF)
cmake_dependent_option (CH_ODBC_PREFER_BUNDLED_FOLLY "Prefer bundled over system variants of Folly library" ON "CH_ODBC_PREFER_BUNDLED_THIRD_PARTIES" OFF)
cmake_dependent_option (CH_ODBC_PREFER_BUNDLED_ICU "Prefer bundled over system variants of ICU library" OFF "CH_ODBC_PREFER_BUNDLED_THIRD_PARTIES" OFF)
option (CH_ODBC_RUNTIME_LINK_STATIC "Link with compiler and language runtime statically" OFF)
option (CH_ODBC_THIRD_PARTY_LINK_STATIC "Link with third party libraries statically" ON)

set (CH_ODBC_DEFAULT_DSN_ANSI "Couchbase DSN (ANSI)" CACHE STRING "Default ANSI DSN name")
set (CH_ODBC_DEFAULT_DSN_UNICODE "Couchbase DSN (Unicode)" CACHE STRING "Default Unicode DSN name")

if (MSVC)
    # This default encoding mode will be overriden by UNICODE, in the corresponding cases.
    add_compile_definitions (SBCS _SBCS)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
endif ()

if (ARCH_FREEBSD)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -U_XOPEN_SOURCE -UPOCO_HAVE_FD_EPOLL")
endif ()

# Make sure that all optimized builds have NDEBUG macro set.
foreach (config Release RelWithDebInfo MinSizeRel)
    foreach (lang CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
        string (TOUPPER "${config}" config_uc)
        set (var "${lang}_${config_uc}")
        set (${var} "${${var}} -DNDEBUG")
    endforeach ()
endforeach ()

# Set BUILD_TYPE_* macro for each of build types.
foreach (config Debug Release RelWithDebInfo MinSizeRel)
    foreach (lang CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
        string (TOUPPER "${config}" config_uc)
        set (var "${lang}_${config_uc}")
        set (${var} "${${var}} -DBUILD_TYPE_${config_uc}")
    endforeach ()
endforeach ()

if (CH_ODBC_RUNTIME_LINK_STATIC)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        foreach (config ${CMAKE_CONFIGURATION_TYPES_UC})
            foreach (lang CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
                set (var "${lang}_${config}")
                string (REPLACE "/MD" "/MT" ${var} "${${var}}")
                string (REPLACE "-MD" "-MT" ${var} "${${var}}")
            endforeach ()
        endforeach ()
    elseif (
        CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
        (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT APPLE)
    )
        if (NOT APPLE AND NOT WIN32 AND NOT ARCH_FREEBSD)
            set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--whole-archive -static-libgcc -static-libstdc++ -Wl,--no-whole-archive")
            set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--whole-archive -static-libgcc -static-libstdc++ -Wl,--no-whole-archive")
        else ()
            set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
            set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")
        endif ()
    else ()
        message (WARNING "linking with compiler and language runtime statically is not supported in the current build environment")
    endif()
endif ()

include (GNUInstallDirs)
include (cmake/arch.cmake)
include (cmake/sanitize.cmake)

find_package (Threads REQUIRED)
find_package (ODBC REQUIRED)

if (CH_ODBC_ENABLE_SSL)
    if (NOT CH_ODBC_PREFER_BUNDLED_SSL)
        if (CH_ODBC_THIRD_PARTY_LINK_STATIC)
            set (OPENSSL_USE_STATIC_LIBS TRUE)
        endif ()

        if (CH_ODBC_RUNTIME_LINK_STATIC)
            set (OPENSSL_MSVC_STATIC_RT TRUE)
        endif ()

        find_package (OpenSSL)
    endif ()
endif ()

if (NOT CH_ODBC_PREFER_BUNDLED_POCO)
    find_package (Poco COMPONENTS Foundation Net NetSSL)
endif ()

if (NOT CH_ODBC_PREFER_BUNDLED_FOLLY)
    message (WARNING "Folly: using system variant of the library currently not supported")
#   find_package (Folly)
endif ()

if (CH_ODBC_USE_ICU)
    if (CH_ODBC_PREFER_BUNDLED_ICU)
        message (WARNING "ICU: using bundled variant of the library currently not supported")
    endif ()
    find_package (ICU COMPONENTS uc REQUIRED)
endif ()

add_subdirectory (contrib EXCLUDE_FROM_ALL)

foreach (config ${CMAKE_CONFIGURATION_TYPES_UC})
    set (CMAKE_${config}_POSTFIX "" CACHE STRING "" FORCE) # Don't append "d" or "md" to output lib name
endforeach ()

set (CMAKE_C_VISIBILITY_PRESET hidden)
set (CMAKE_CXX_VISIBILITY_PRESET hidden)
set (CMAKE_VISIBILITY_INLINES_HIDDEN 1)

include (CheckIPOSupported)
check_ipo_supported (RESULT ipo_supported LANGUAGES C CXX)
if (ipo_supported)
    if (NOT DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION) # respect value prodivided by user
        set (CMAKE_INTERPROCEDURAL_OPTIMIZATION 1)
        set (CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG 0)
    endif ()
endif ()

add_subdirectory (driver)

# Installation
set (CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT RuntimeLibraries)
include (InstallRequiredSystemLibraries)

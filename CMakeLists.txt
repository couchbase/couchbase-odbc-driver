cmake_minimum_required (VERSION 3.13.5)

set (CMAKE_MODULE_PATH_orig ${CMAKE_MODULE_PATH})
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")
include (cmake/prevent_in_source_builds.cmake)
include (CBDeps)

# PRODUCT_VERSION logic:
# 1. If PRODUCT_VERSION is already defined, that means it's either cached
#    or was explicitly specified with -D on the command-line. In either
#    case, force it into the cache so it will be remembered in future runs.
# 2. Otherwise, we do NOT want to cache PRODUCT_VERSION, which will force
#    CMake to re-discover it every time. In that case, the build number
#    will always be "0000", and:
#    2a. If "`" can be found, get the version from the manifest.
#    2b. Otherwise, use the fallback version "0.0.0".
IF (DEFINED PRODUCT_VERSION)
  SET (PRODUCT_VERSION "${PRODUCT_VERSION}" CACHE STRING "Product version with build number" FORCE)
ELSE ()
  SET (_product_version)
  IF (WIN32)
    SET (_repo_cmd repo.cmd)
  ELSE ()
    SET (_repo_cmd repo)
  ENDIF ()
  IF (EXISTS "${CMAKE_SOURCE_DIR}/../manifest.xml")
    # Building from a source tarball - use that manifest
    EXECUTE_PROCESS(
      COMMAND python cmake/get_version.py "../manifest.xml"
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      OUTPUT_VARIABLE _output
      ERROR_VARIABLE _err_output
      RESULTS_VARIABLE _result
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    IF (_result STREQUAL "0")
      SET (PRODUCT_VERSION ${_output})
    ENDIF ()
  ELSE ()
    # Try to pull from repo
    EXECUTE_PROCESS(
      COMMAND ${_repo_cmd} manifest -r
      COMMAND python cmake/get_version.py
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE _output
      ERROR_VARIABLE _err_output
      RESULTS_VARIABLE _result
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    IF (_result STREQUAL "0;0")
      SET (PRODUCT_VERSION ${_output})
    ENDIF ()
  ENDIF ()
  IF (NOT DEFINED PRODUCT_VERSION)
    SET (PRODUCT_VERSION "0.0.0")
  ENDIF ()
ENDIF ()
MESSAGE (STATUS "Building Couchbase ODBC Driver version ${PRODUCT_VERSION}")

project (
    couchbase-odbc
    VERSION ${PRODUCT_VERSION}
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
option (CH_ODBC_ENABLE_INSTALL "Enable install targets (required for packaging)" ON)
cmake_dependent_option (CH_ODBC_USE_ICU "Use ICU library, instead of C++ STD, for Unicode conversions" ON "NOT MSVC" OFF)
option (CH_ODBC_PREFER_BUNDLED_THIRD_PARTIES "Prefer bundled over system variants of third party libraries" ON)
cmake_dependent_option (CH_ODBC_PREFER_BUNDLED_POCO "Prefer bundled over system variants of Poco library" ON "CH_ODBC_PREFER_BUNDLED_THIRD_PARTIES" OFF)
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


if (CH_ODBC_ENABLE_INSTALL)
    install(
        FILES
            packaging/couchbase-odbc.tdc.sample
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/config
        COMPONENT Documentation
    )
    set (CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT RuntimeLibraries)
    include (InstallRequiredSystemLibraries)

    include (CPackComponent)

    # Remove empty/irrelevant components, mostly brought by third-party projects.
    get_cmake_property (CPACK_COMPONENTS_ALL COMPONENTS)
    list (REMOVE_ITEM CPACK_COMPONENTS_ALL "Devel")

    cpack_add_component_group (ANSIGroup
        DISPLAY_NAME "ANSI ${ARCH_BITS}-bit Driver"
        DESCRIPTION "Couchbase ODBC Driver (ANSI, ${ARCH_BITS}-bit)"
        EXPANDED
    )

    cpack_add_component (ANSIDriver
        DISPLAY_NAME "Driver"
        DESCRIPTION "Couchbase ODBC Driver (ANSI, ${ARCH_BITS}-bit)"
        REQUIRED
        GROUP ANSIGroup
    )

    cpack_add_component (ANSIDriverDebugSymbols
        DISPLAY_NAME "Debug symbols"
        DESCRIPTION "Debug symbols (PDB) for the Driver (ANSI, ${ARCH_BITS}-bit)"
        DISABLED
        DEPENDS ANSIDriver
        GROUP ANSIGroup
    )

    cpack_add_component_group (UnicodeGroup
        DISPLAY_NAME "Unicode ${ARCH_BITS}-bit Driver"
        DESCRIPTION "Couchbase ODBC Driver (Unicode, ${ARCH_BITS}-bit)"
        EXPANDED
    )

    cpack_add_component (UnicodeDriver
        DISPLAY_NAME "Driver"
        DESCRIPTION "Couchbase ODBC Driver (Unicode, ${ARCH_BITS}-bit)"
        DEPENDS ANSIDriver
        GROUP UnicodeGroup
    )

    cpack_add_component (UnicodeDriverDebugSymbols
        DISPLAY_NAME "Debug symbols"
        DESCRIPTION "Debug symbols (PDB) for Driver (Unicode, ${ARCH_BITS}-bit)"
        DISABLED
        DEPENDS UnicodeDriver
        GROUP UnicodeGroup
    )

    cpack_add_component (RuntimeLibraries
        DISPLAY_NAME "Runtime Libraries"
        DESCRIPTION "System/language redistributable runtime libraries"
        DEPENDS ANSIDriver
    )

    if (WIN32 AND NOT UNIX)
        set (CPACK_GENERATOR "ZIP;WIX")
    elseif (APPLE)
        set (CPACK_GENERATOR "TXZ")
    elseif (UNIX_RHEL OR UNIX_FEDORA OR UNIX_CENTOS)
        set (CPACK_GENERATOR "TGZ;TXZ;RPM")
    elseif (UNIX_DEBIAN OR UNIX_UBUNTU)
        set (CPACK_GENERATOR "TGZ;TXZ") # TODO: add DEB
    else ()
        set (CPACK_GENERATOR "TGZ")
    endif ()

    if (WIN32 AND NOT UNIX)
        set (CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
    else ()
        set (CPACK_PACKAGE_INSTALL_DIRECTORY "/usr/local")
    endif ()

    set (CPACK_PROJECT_CONFIG_FILE "${PROJECT_SOURCE_DIR}/CPackLists.txt")

    set (CPACK_PACKAGE_VENDOR "Couchbase")
    set (CPACK_PACKAGE_DESCRIPTION "The official ODBC driver implementation for accessing Couchbase as a data source.")
    set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Couchbase ODBC Driver (${ARCH_BITS}-bit)")
    set (CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/packaging/Readme.rtf")
    set (CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/packaging/License.rtf")
    set (CPACK_PACKAGE_CHECKSUM "SHA256")
    set (CPACK_PACKAGE_RELOCATABLE ON)

    set (CPACK_RPM_COMPONENT_INSTALL OFF)
    set (CPACK_RPM_FILE_NAME RPM-DEFAULT)
    set (CPACK_RPM_PACKAGE_RELEASE_DIST ON)
    set (CPACK_RPM_PACKAGE_LICENSE "ASL 2.0")
    set (CPACK_RPM_PACKAGE_GROUP "Applications/Databases")
    set (CPACK_RPM_COMPRESSION_TYPE "gzip")
    set (CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")
    set (CPACK_RPM_CHANGELOG_FILE "${PROJECT_SOURCE_DIR}/packaging/Changelog.txt")

    if ("${ODBC_PROVIDER}" STREQUAL "UnixODBC")
        set (CPACK_RPM_PACKAGE_REQUIRES "unixODBC")
    elseif ("${ODBC_PROVIDER}" STREQUAL "iODBC")
        set (CPACK_RPM_PACKAGE_REQUIRES "libiodbc")
    endif ()

    set (CPACK_RPM_DEBUGINFO_PACKAGE OFF)
    set (CPACK_RPM_PACKAGE_SOURCES OFF)

    set (CPACK_WIX_ROOT_FEATURE_TITLE "Couchbase ODBC Driver")
    set (CPACK_WIX_ROOT_FEATURE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")
    set (CPACK_WIX_PATCH_FILE "${PROJECT_SOURCE_DIR}/packaging/RegConfig.patch.wxs")
    if ("${ARCH_BITS}" STREQUAL "32")
        set (CPACK_WIX_PRODUCT_GUID "14819BE8-ADDD-41FB-AF0B-9EBEEFC0E444")
        set (CPACK_WIX_UPGRADE_GUID "4BE0FED2-BDF4-40D5-AD2B-D3BF1C2BE079")
    elseif ("${ARCH_BITS}" STREQUAL "64")
        set (CPACK_WIX_PRODUCT_GUID "837CD50F-6091-435E-83E7-81C3C7F43B64")
        set (CPACK_WIX_UPGRADE_GUID "C7F2F2C0-7438-4729-AD76-A6A7C2F85426")
    endif ()

    include (CPack)
endif ()

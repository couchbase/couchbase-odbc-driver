# Downloads the 'cbdep' utility and defines a macro "cbdep_install()"
# to make use of it
# Note: this assumes we are on Windows in several places

set (CBDEP_VERSION 1.1.8)

# Utilize cbdep's own cache dir
if (WIN32)
  set (_cbdepcache_dir "$ENV{HOMEDRIVE}/$ENV{HOMEPATH}/.cbdepcache")
else ()
  set (_cbdepcache_dir "$ENV{HOME}/.cbdepcache")
endif ()
if (NOT IS_DIRECTORY "${_cbdepcache_dir}")
  file (MAKE_DIRECTORY "${_cbdepcache_dir}")
endif ()

if (WIN32)
  set (_cbdep_host "windows")
  set (_cbdep_bin_arch "x86_64")
  set (_cbdep_ext ".exe")
elseif (APPLE)
  set (_cbdep_host "darwin")
  if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
    set (_cbdep_bin_arch "arm64")
  else ()
    set (_cbdep_bin_arch "x86_64")
  endif ()
  set (_cbdep_ext "")
elseif (UNIX)
  set (_cbdep_host "linux")
  if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
    set (_cbdep_bin_arch "arm64")
  else ()
    set (_cbdep_bin_arch "x86_64")
  endif ()
  set (_cbdep_ext "")
else ()
  message (FATAL_ERROR "Unsupported host platform for cbdep")
endif ()

set (CBDEP_EXE "${_cbdepcache_dir}/cbdep-${CBDEP_VERSION}-${_cbdep_host}-${_cbdep_bin_arch}${_cbdep_ext}")
if (NOT EXISTS "${CBDEP_EXE}")
  set (_cbdep_url "https://packages.couchbase.com/cbdep/${CBDEP_VERSION}/cbdep-${CBDEP_VERSION}-${_cbdep_host}-${_cbdep_bin_arch}${_cbdep_ext}")
  message (STATUS "Downloading cbdep ${CBDEP_VERSION}...")
  file (DOWNLOAD "${_cbdep_url}" "${CBDEP_EXE}" STATUS _stat SHOW_PROGRESS)
  list (GET _stat 0 _retval)
  if (_retval)
    file (REMOVE "${CBDEP_EXE}")
    list (GET _stat 1 _message)
    message (
      FATAL_ERROR "Error downloading ${cbdep_url}: ${_message} (${_retval})"
    )
  endif ()
endif ()

if (NOT WIN32)
  execute_process (COMMAND chmod +x "${CBDEP_EXE}")
endif ()

include (ParseArguments)

# Generic function for installing a cbdep (2.0) package to a given directory
# Required arguments:
#   PACKAGE - package to install
#   VERSION - version number of package (must be understood by 'cbdep' tool)
# Optional arguments:
#   INSTALL_DIR - where to install to; defaults to CMAKE_CURRENT_BINARY_DIR
MACRO (CBDEP_INSTALL)
  PARSE_ARGUMENTS (cbdep "" "INSTALL_DIR;PACKAGE;VERSION" "" ${ARGN})
  IF (NOT cbdep_INSTALL_DIR)
    SET (cbdep_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  ENDIF ()
  IF(NOT IS_DIRECTORY "${cbdep_INSTALL_DIR}/${cbdep_PACKAGE}-${cbdep_VERSION}")
    MESSAGE (STATUS "Downloading and caching ${cbdep_PACKAGE}-${cbdep_VERSION}")
    EXECUTE_PROCESS (
      COMMAND "${CBDEP_EXE}" -p ${_cbdep_host}
        install -d "${cbdep_INSTALL_DIR}"
        ${cbdep_PACKAGE} ${cbdep_VERSION}
      RESULT_VARIABLE _cbdep_result
      OUTPUT_VARIABLE _cbdep_out
      ERROR_VARIABLE _cbdep_out
    )
    IF (_cbdep_result)
      FILE (REMOVE_RECURSE "${cbdep_INSTALL_DIR}")
      MESSAGE (FATAL_ERROR "Failed installing cbdep ${cbdep_PACKAGE} ${cbdep_VERSION}: ${_cbdep_out}")
    ENDIF ()
  ENDIF()
ENDMACRO (CBDEP_INSTALL)

include(FindPackageHandleStandardArgs)

function(SUBDIRLIST result input)
  set(res "")
  foreach(dir ${${input}})
    file(GLOB curdir ${dir})
    list(APPEND dirlist ${curdir})
  endforeach()

  while(dirlist)
    list(GET dirlist 0 curdir)
    list(APPEND res ${curdir})
    file(GLOB children ${curdir}/*)
    foreach(child ${children})
      if(IS_DIRECTORY ${child})
        list(APPEND dirlist ${child})
      endif()
    endforeach()
    list(REMOVE_ITEM dirlist ${curdir})
  endwhile()
  set(${result} ${res} PARENT_SCOPE)
endfunction()

function(PACKAGE_FINDING_HELPER PACKAGE_NAME HEADER_FILE)
  unset(${PACKAGE_NAME}_INCLUDE_DIR CACHE)
  unset(${PACKAGE_NAME}_LIBRARY CACHE)

  find_path(
    ${PACKAGE_NAME}_INCLUDE_DIR
    ${HEADER_FILE}
    HINTS ${${PACKAGE_NAME}_SRCH_DIRS}
    NO_DEFAULT_PATH
    )

  if(DEFINED ${PACKAGE_NAME}_LIBRARY_NAMES)
    find_library(
      ${PACKAGE_NAME}_LIBRARY
      NAMES ${${PACKAGE_NAME}_LIBRARY_NAMES}
      PATHS ${${PACKAGE_NAME}_SRCH_DIRS}
      NO_DEFAULT_PATH
    )

    string(COMPARE NOTEQUAL
      "${PACKAGE_NAME}_LIBRARY-NOTFOUND"
      ${${PACKAGE_NAME}_LIBRARY}

      ${PACKAGE_NAME}_LIBRARY_FOUND)

    if(NOT ${${PACKAGE_NAME}_LIBRARY_FOUND})
      message(FATAL_ERROR "Could not find ${${PACKAGE_NAME}_LIBRARY_NAMES}")
    endif()
  endif()

  list(APPEND ${PACKAGE_NAME}_REQUIRED_VARS ${PACKAGE_NAME}_INCLUDE_DIR)
  if(DEFINED ${PACKAGE_NAME}_LIBRARY_NAMES)
    list(APPEND ${PACKAGE_NAME}_REQUIRED_VARS ${PACKAGE_NAME}_LIBRARY)
  endif()
  find_package_handle_standard_args(
    ${PACKAGE_NAME}
    REQUIRED_VARS ${${PACKAGE_NAME}_REQUIRED_VARS}
    FAIL_MESSAGE
    "Could not find package ${PACKAGE_NAME}. Is MESOS_BUILD_DIR correctly set?"
    )
  if(DEFINED ${PACKAGE_NAME}_LIBRARY)
    message(STATUS "${PACKAGE_NAME} library: ${${PACKAGE_NAME}_LIBRARY}")
  endif()
  if(DEFINED ${PACKAGE_NAME}_INCLUDE_DIR)
    message(STATUS "${PACKAGE_NAME} header: ${${PACKAGE_NAME}_INCLUDE_DIR}")
  endif()
  set(${PACKAGE_NAME}_LIBRARY ${${PACKAGE_NAME}_LIBRARY} PARENT_SCOPE)
endfunction()

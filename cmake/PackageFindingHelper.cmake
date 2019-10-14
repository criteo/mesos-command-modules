include(FindPackageHandleStandardArgs)

function(SUBDIRLIST result input)
  file(GLOB curdir ${input})
  set(res "")
  list(APPEND dirlist ${curdir})

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
  unset(${PACKAGE_NAME}_LIBRARY_DIRS CACHE)

  find_path(
    ${PACKAGE_NAME}_INCLUDE_DIR
    ${HEADER_FILE}
    HINTS ${${PACKAGE_NAME}_SRCH_DIRS})

  foreach(LIBRARY_NAME ${${PACKAGE_NAME}_LIBRARY_NAMES})
    unset(${PACKAGE_NAME}_LIB_PATH CACHE)

    find_path(
      ${PACKAGE_NAME}_LIB_PATH
      ${${PACKAGE_NAME}_LIBRARY_NAMES}
      HINTS ${${PACKAGE_NAME}_SRCH_DIRS}
      )

    string(COMPARE NOTEQUAL
      "${PACKAGE_NAME}_LIB_PATH-NOTFOUND"
      ${${PACKAGE_NAME}_LIB_PATH}

      ${PACKAGE_NAME}_LIB_PATH_FOUND)

    if(NOT ${${PACKAGE_NAME}_LIB_PATH_FOUND})
      message(FATAL_ERROR "Could not find ${LIBRARY_NAME}")
    endif()

    list(APPEND ${PACKAGE_NAME}_LIBRARY_DIRS ${${PACKAGE_NAME}_LIB_PATH})
  endforeach()

  list(APPEND ${PACKAGE_NAME}_REQUIRED_VARS ${PACKAGE_NAME}_INCLUDE_DIR)
  if(DEFINED ${PACKAGE_NAME}_LIBRARY_NAMES)
    list(APPEND ${PACKAGE_NAME}_REQUIRED_VARS ${PACKAGE_NAME}_LIBRARY_DIRS)
  endif()
  find_package_handle_standard_args(
    ${PACKAGE_NAME}
    REQUIRED_VARS ${${PACKAGE_NAME}_REQUIRED_VARS}
    FAIL_MESSAGE
    "Could not find package ${PACKAGE_NAME}. Is MESOS_BUILD_DIR correctly set?"
    )
  if(DEFINED ${PACKAGE_NAME}_LIBRARY_DIRS)
    message(STATUS "${PACKAGE_NAME} library: ${${PACKAGE_NAME}_LIBRARY_DIRS}")
  endif()
  if(DEFINED ${PACKAGE_NAME}_INCLUDE_DIR)
    message(STATUS "${PACKAGE_NAME} header: ${${PACKAGE_NAME}_INCLUDE_DIR}")
  endif()
  set(${PACKAGE_NAME}_LIBRARY_DIRS ${${PACKAGE_NAME}_LIBRARY_DIRS} PARENT_SCOPE)
endfunction()

find_program(
  CLANG_FORMAT_EXE
  NAMES "clang-format"
  DOC "Path to clang-format executable"
  )

if(NOT CLANG_FORMAT_EXE)
  message(STATUS "clang-format not installed.")
else()
  message(STATUS "clang-format found: ${CLANG_FORMAT_EXE}")
  add_custom_target(
        clang-format
        COMMAND "${CLANG_FORMAT_EXE}"
        -style=file
        -i
        ${ALL_SOURCES}
  )
endif()

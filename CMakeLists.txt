cmake_minimum_required(VERSION 2.8)

set(PROJECT_NAME mesos_command_modules)
project(${PROJECT_NAME})

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -Wall -Wno-macro-redefined -std=c++11")

find_package(PkgConfig REQUIRED)

if(DEFINED ENV{MESOS_BUILD_DIR})
  set(MESOS_BUILD_DIR $ENV{MESOS_BUILD_DIR})
else()
  set(MESOS_BUILD_DIR "${PWD}")
endif()

# Required include path lookup in Build dir
file(GLOB BOOST_SRCH_PATH "${MESOS_BUILD_DIR}/3rdparty/boost-*")
find_path(BOOST_INCLUDE_DIR NAMES "boost/version.hpp" PATHS ${BOOST_SRCH_PATH} NO_DEFAULT_PATH)

file(GLOB PROTO_SRCH_PATH "${MESOS_BUILD_DIR}/3rdparty/protobuf-*/src")
find_path(PROTOBUF_INCLUDE_DIR NAMES "google/protobuf/stubs/common.h" PATHS ${PROTO_SRCH_PATH} NO_DEFAULT_PATH)

file(GLOB GLOG_SRCH_PATH "${MESOS_BUILD_DIR}/3rdparty/glog-*/src")
find_path(GLOG_INCLUDE_DIR NAMES "glog/logging.h" PATHS ${GLOG_SRCH_PATH} NO_DEFAULT_PATH)

file(GLOB PICOJSON_SRCH_PATH "${MESOS_BUILD_DIR}/3rdparty/picojson-*")
find_path(PICOJSON_INCLUDE_DIR NAMES "picojson.h" PATHS ${PICOJSON_SRCH_PATH} NO_DEFAULT_PATH)

include_directories(
  ./src
  ${BOOST_INCLUDE_DIR}
  ${GLOG_INCLUDE_DIR}
  ${PROTOBUF_INCLUDE_DIR}
  ${PICOJSON_INCLUDE_DIR}
  ${MESOS_BUILD_DIR}/3rdparty/stout/include
  ${MESOS_BUILD_DIR}/3rdparty/libprocess/include
  )

pkg_check_modules(MESOS QUIET mesos)

if (NOT MESOS_FOUND) # Assume build tree has not been installed yet
  set(MESOS_CFLAGS -DPICOJSON_USE_INT64 -D__STDC_FORMAT_MACROS)
  set(MESOS_INCLUDE_DIRS ${MESOS_BUILD_DIR}/include)
  set(MESOS_LIBRARY_DIRS ${MESOS_BUILD_DIR}/src/.libs)
  set(MESOS_LIBRARIES mesos)
endif()



set(MODULES_SOURCES
  ${CMAKE_SOURCE_DIR}/src/CommandHook.cpp
  ${CMAKE_SOURCE_DIR}/src/CommandIsolator.cpp
  ${CMAKE_SOURCE_DIR}/src/CommandRunner.cpp
  ${CMAKE_SOURCE_DIR}/src/ConfigurationParser.cpp
  ${CMAKE_SOURCE_DIR}/src/ModulesFactory.cpp
)

set(MODULES_HEADERS
  ${CMAKE_SOURCE_DIR}/src/Command.hpp
  ${CMAKE_SOURCE_DIR}/src/CommandHook.hpp
  ${CMAKE_SOURCE_DIR}/src/CommandIsolator.hpp
  ${CMAKE_SOURCE_DIR}/src/CommandRunner.hpp
  ${CMAKE_SOURCE_DIR}/src/ConfigurationParser.hpp
  ${CMAKE_SOURCE_DIR}/src/Helpers.hpp
  ${CMAKE_SOURCE_DIR}/src/Logger.hpp
  ${CMAKE_SOURCE_DIR}/src/ModulesFactory.hpp
)

set(ALL_SOURCES
  ${MODULES_SOURCES}
  ${MODULES_HEADERS}
)

set(TEST_SOURCES
  ${CMAKE_SOURCE_DIR}/tests/main.cpp
  ${CMAKE_SOURCE_DIR}/tests/CommandRunnerTest.cpp
  ${CMAKE_SOURCE_DIR}/tests/CommandHookTest.cpp
  ${CMAKE_SOURCE_DIR}/tests/CommandIsolatorTest.cpp
  ${CMAKE_SOURCE_DIR}/tests/ConfigurationParserTest.cpp
  ${CMAKE_SOURCE_DIR}/tests/ModulesFactoryTest.cpp
)

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

add_compile_options(${MESOS_CFLAGS})
include_directories(${MESOS_INCLUDE_DIRS})
link_directories(${MESOS_LIBRARY_DIRS})
link_libraries(${MESOS_LIBRARIES})

add_library(${PROJECT_NAME} SHARED ${MODULES_SOURCES})

# Unit Tests building & execution

SET(TEST_BINARY_NAME test_mesos_command_modules)
add_executable(${TEST_BINARY_NAME}
  ${TEST_SOURCES}
)

file(GLOB GTEST_SRCH_PATH "${MESOS_BUILD_DIR}/3rdparty/googletest-*/googletest/include")
find_path(GTEST_INCLUDE_DIR NAMES "gtest/gtest.h" PATHS ${GTEST_SRCH_PATH} NO_DEFAULT_PATH)

target_include_directories(${TEST_BINARY_NAME} PUBLIC ${GTEST_INCLUDE_DIR} tests)
target_link_libraries(${TEST_BINARY_NAME} ${PROJECT_NAME} ${MESOS_BUILD_DIR}/3rdparty/.libs/libgmock.a pthread)
add_custom_target(check COMMAND ${TEST_BINARY_NAME} ${CMAKE_SOURCE_DIR}/tests/scripts/)


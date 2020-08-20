cmake_minimum_required(VERSION 3.13)

set(PROJECT_NAME mesos_command_modules)
project(${PROJECT_NAME})

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_MACOSX_RPATH 1)
set(
  CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -O0 -Wall -Wno-macro-redefined -std=c++11"
  )

find_package(PkgConfig REQUIRED)

if(DEFINED ENV{MESOS_BUILD_DIR} AND DEFINED ENV{MESOS_ROOT_DIR})
  set(MESOS_BUILD_DIR $ENV{MESOS_BUILD_DIR})
  set(MESOS_ROOT_DIR $ENV{MESOS_ROOT_DIR})
else()
  message(FATAL_ERROR "You must set MESOS_BUILD_DIR and MESOS_ROOT_DIR")
endif()

# Required include path lookup in Build dir
find_package(BOOST REQUIRED QUIET)
find_package(GLOG REQUIRED QUIET)
find_package(GLOGBUILD REQUIRED QUIET)
find_package(GTEST REQUIRED QUIET)
find_package(PICOJSON REQUIRED QUIET)
find_package(PROTOBUF REQUIRED QUIET)
find_package(RAPIDJSON REQUIRED QUIET)

include_directories(
  ./src
  ${BOOST_INCLUDE_DIR}
  ${GLOGBUILD_INCLUDE_DIR}
  ${GLOG_INCLUDE_DIR}
  ${PICOJSON_INCLUDE_DIR}
  ${PROTOBUF_INCLUDE_DIR}
  ${RAPIDJSON_INCLUDE_DIR}
  ${MESOS_ROOT_DIR}/3rdparty/stout/include
  ${MESOS_ROOT_DIR}/3rdparty/libprocess/include
  )

pkg_check_modules(MESOS QUIET mesos)

if(NOT MESOS_FOUND) # Assume build tree has not been installed yet
  set(MESOS_CFLAGS -DPICOJSON_USE_INT64 -D__STDC_FORMAT_MACROS)
  set(
    MESOS_INCLUDE_DIRS ${MESOS_BUILD_DIR}/include ${MESOS_ROOT_DIR}/include
    )
  set(MESOS_LIBRARY_DIRS ${MESOS_BUILD_DIR}/src/.libs ${MESOS_ROOT_DIR}/src/.libs)
  set(MESOS_LIBRARIES mesos)
endif()



set(MODULES_SOURCES
  ${CMAKE_SOURCE_DIR}/src/CommandHook.cpp
  ${CMAKE_SOURCE_DIR}/src/CommandIsolator.cpp
  ${CMAKE_SOURCE_DIR}/src/CommandRunner.cpp
  ${CMAKE_SOURCE_DIR}/src/RunningContext.cpp
  ${CMAKE_SOURCE_DIR}/src/ConfigurationParser.cpp
  ${CMAKE_SOURCE_DIR}/src/ModulesFactory.cpp
  ${CMAKE_SOURCE_DIR}/src/OpportunisticResourceEstimator.cpp
)

set(MODULES_HEADERS
  ${CMAKE_SOURCE_DIR}/src/Command.hpp
  ${CMAKE_SOURCE_DIR}/src/CommandHook.hpp
  ${CMAKE_SOURCE_DIR}/src/CommandIsolator.hpp
  ${CMAKE_SOURCE_DIR}/src/CommandRunner.hpp
  ${CMAKE_SOURCE_DIR}/src/RunningContext.hpp
  ${CMAKE_SOURCE_DIR}/src/ConfigurationParser.hpp
  ${CMAKE_SOURCE_DIR}/src/Helpers.hpp
  ${CMAKE_SOURCE_DIR}/src/Logger.hpp
  ${CMAKE_SOURCE_DIR}/src/ModulesFactory.hpp
  ${CMAKE_SOURCE_DIR}/src/OpportunisticResourceEstimator.hpp
)

set(ALL_SOURCES
  ${MODULES_SOURCES}
  ${MODULES_HEADERS}
)

include(ClangFormatCheck)

add_compile_options(${MESOS_CFLAGS})
include_directories(${MESOS_INCLUDE_DIRS})
link_directories(${MESOS_LIBRARY_DIRS})
link_libraries(${MESOS_LIBRARIES})
add_library(${PROJECT_NAME} SHARED ${MODULES_SOURCES})

# Unit Tests building & execution
include(UnitTestsCheck)

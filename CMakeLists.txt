cmake_minimum_required(VERSION 3.10)
project(Parallel-Plotter VERSION 0.1.0)

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(CMAKE_CXX_FLAGS_RELEASE "/MT")
  set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
endif()

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo)
ENDIF()

execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(
  "${PROJECT_SOURCE_DIR}/src/config.h.in"
  "${PROJECT_BINARY_DIR}/src/config.h"
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(COMMON_WARN "-Wall -Werror -Wextra -Wstrict-aliasing=2 -Wno-unknown-pragmas -Wno-unused-parameter")
set(COMMON_WARN "${COMMON_WARN} -Wno-unused-variable -Wno-unused-function -Wno-deprecated-declarations -Wno-write-strings")
set(COMMON_WARN "${COMMON_WARN} -Wno-ignored-qualifiers -Wno-ignored-attributes -Wno-parentheses")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(COMMON_WARN "${COMMON_WARN} -Wno-implicit-fallthrough")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_C_FLAGS} ${COMMON_WARN}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS} ${COMMON_WARN}  -faligned-new -fno-enforce-eh-specs -fnothrow-opt")
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS} ${COMMON_WARN}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_COMPUTE_USE_CPP11")
endif()

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib")
file(MAKE_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")
file(MAKE_DIRECTORY "${LIBRARY_OUTPUT_PATH}")
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/kernel")
file(COPY kernel/kernel.cl  DESTINATION "${PROJECT_BINARY_DIR}/kernel")

# untill boost 1.68 compute is fixed
find_package(Boost 1.68 REQUIRED filesystem)
include_directories("${Boost_INCLUDE_DIRS}")

find_package(OpenCL REQUIRED)

include_directories("${PROJECT_BINARY_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${PROJECT_SOURCE_DIR}/contrib/")
include_directories("${PROJECT_SOURCE_DIR}/contrib/cpp-optparse/")
include_directories("${PROJECT_SOURCE_DIR}/contrib/spdlog/include/")

add_subdirectory(src)
add_subdirectory(contrib)
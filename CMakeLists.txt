cmake_minimum_required(VERSION 3.9)
project(sqlite_cpp)

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
	set(CMAKE_CXX_FLAGS_DEBUG "-Og -g -lgcov --coverage")
endif(MSVC)

set(SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(TEST_DIR ${CMAKE_CURRENT_LIST_DIR}/tests)
set(SQLITE_DIR ${CMAKE_CURRENT_LIST_DIR}/lib)

set(SOURCES
	${SOURCE_DIR}/sqlite_cpp.cpp
)
set(TEST_SOURCES
	${TEST_DIR}/catch.hpp
	${TEST_DIR}/main.cpp
	${TEST_DIR}/test_misuse.cpp
)

include_directories(${SOURCE_DIR})
include_directories(${SQLITE_DIR})
include_directories(${TEST_DIR})

## SQLite
add_library(sqlite STATIC
	${SQLITE_DIR}/sqlite3.c
)
set_target_properties(sqlite PROPERTIES LINKER_LANGUAGE C)

## Main Library
add_library(sqlite_cpp STATIC ${SOURCES})
set_target_properties(sqlite_cpp PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(sqlite_cpp sqlite)
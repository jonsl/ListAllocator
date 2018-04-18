CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(list_alloc C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

SET(SRCS
        src/core/config.h
        src/core/container_types.h
        src/core/list.h
        src/core/list_alloc.cpp
        src/core/list_alloc.h
        src/core/list_arena.h
        src/core/list_arena.tcc
        src/main.cpp)

SET(INCLUDE_DIRS
        src/
        src/core/)

ADD_EXECUTABLE(list_alloc ${SRCS})

TARGET_INCLUDE_DIRECTORIES(list_alloc PRIVATE ${INCLUDE_DIRS})


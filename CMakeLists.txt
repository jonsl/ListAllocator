CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(magn C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

SET (SRCS
        src/main.cpp
        src/core/Allocator.h
        src/core/Allocator.ipp
        src/core/ListAllocator.cpp
        src/core/ListAllocator.h
        src/core/List.h
        src/core/Config.h)

SET(INCLUDE_DIRS src/
        src/core/)

ADD_EXECUTABLE(ListAllocator ${SRCS})

TARGET_INCLUDE_DIRECTORIES(ListAllocator PRIVATE ${INCLUDE_DIRS} )


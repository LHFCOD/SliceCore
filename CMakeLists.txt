# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)
# 项目信息
project (SliceCore)
# 并将名称保存到 DIR_SRCS 变量
aux_source_directory(./src DIR_SRCS)
aux_source_directory(. DIR_SRCS)
list(REMOVE_ITEM DIR_SRCS "./src/MicroComDocIO.cpp") 
include_directories($ENV{BOOST_ROOT})
add_compile_options(-std=c++11 -fPIC )
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(CMAKE_LINKER_FLAGS "-static -pthread -lrt -ldl")
link_directories($ENV{BOOST_ROOT}/stage/lib)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
find_package(Threads REQUIRED)

set(TARGET mds)
add_executable(${TARGET} ${DIR_SRCS})
# list(REMOVE_ITEM DIR_SRCS "test.cpp") 
# add_library(${TARGET} SHARED ${DIR_SRCS})
target_link_libraries(${TARGET} Threads::Threads libstdc++.a libboost_iostreams.a )
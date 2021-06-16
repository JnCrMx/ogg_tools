cmake_minimum_required(VERSION 3.10)

project(ogg_tools)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include/)

add_executable(oggdump src/oggdump.cpp)
add_executable(oggunwind src/oggunwind.cpp)
add_executable(oggreplay src/oggreplay.cpp)

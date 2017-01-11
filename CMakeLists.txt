cmake_minimum_required(VERSION 3.6)
project(ChannelTester)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(SOURCE_FILES main.cpp cmd.cpp data.cpp noise.cpp tester.cpp utils.cpp)
add_executable(ChannelTester ${SOURCE_FILES})
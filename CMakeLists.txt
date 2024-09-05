cmake_minimum_required(VERSION 3.21)
project(multicast-test)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()
set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(Boost COMPONENTS system filesystem REQUIRED)
include_directories(AFTER ${Boost_INCLUDE_DIRS})
link_directories ( ${Boost_LIBRARY_DIRS} )


add_executable(multicast_test multicast_test.cpp)
#target_link_libraries(discovery_test ${Boost_LIBRARIES})

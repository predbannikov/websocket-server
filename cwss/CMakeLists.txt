cmake_minimum_required(VERSION 3.5)

project(cwss VERSION 2.3)

if (NOT CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    message(STATUS "This project has a top-level one called [${CMAKE_PROJECT_NAME}]")
else()
    message(STATUS "This project is a top-level one")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

configure_file(wssconfig.h.in wssconfig.h)

find_package(Boost 1.78 COMPONENTS program_options REQUIRED)
find_package(Threads REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
include_directories( ${Boost_INCLUDE_DIR} )


add_library(${PROJECT_NAME} wss.h wss.cpp )

target_link_libraries( ${PROJECT_NAME} LINK_PUBLIC  Threads::Threads ${Boost_LIBRARIES}  boost_system boost_thread)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
target_include_directories( ${PROJECT_NAME} PUBLIC
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
    )

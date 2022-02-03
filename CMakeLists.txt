cmake_minimum_required(VERSION 3.5)

# Disable in-source builds to prevent source tree corruption.
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "
FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.
")
endif()

project(wss-serv VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
FIND_PACKAGE( Boost 1.78 COMPONENTS program_options REQUIRED )
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

option(USE_LIBWSS "Use castom websocket server library" ON)

configure_file(wsslibConfig.h.in wsslibConfig.h)

if(USE_LIBWSS) 
    add_subdirectory(libwss)
    list(APPEND EXTRA_LIBS libwss)
endif()


add_executable(wss-serv main.cpp)


target_link_libraries( wss-serv LINK_PUBLIC ${EXTRA_LIBS} Threads::Threads ${Boost_LIBRARIES}  boost_system boost_thread)

target_include_directories(wss-serv PUBLIC 
                            "${PROJECT_BINARY_DIR}"
                            )

enable_testing()

add_test(NAME Runs COMMAND wss-serv)

cmake_minimum_required(VERSION 3.5)

project(wssserver VERSION 4.5)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_LIBWSS "Use custom websocket server library" ON)

configure_file(wsslibConfig.h.in wsslibConfig.h)

add_executable(${PROJECT_NAME} main.cpp )

if(USE_LIBWSS)
    add_subdirectory(cwss)
    list(APPEND EXTRA_LIBS cwss)
endif()

target_link_libraries(${PROJECT_NAME}
        "${EXTRA_LIBS}"
    )

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${PROJECT_BINARY_DIR}
    )









#get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
#foreach(dir ${dirs})
#  message(STATUS "dir='${dir}'")
#endforeach()

#enable_testing()

#add_test(NAME Runs COMMAND wss-serv)

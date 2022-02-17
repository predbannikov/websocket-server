cmake_minimum_required(VERSION 3.5)
project(wssserver VERSION 4.5 LANGUAGES CXX)
get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)



###################################################################################################
##
##      Опции компиляции
##
###################################################################################################

if((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
#    add_compile_options(
#        -Werror

#        -Wall
#        -Wextra
#        -Wpedantic

#        -Wcast-align
#        -Wcast-qual
#        -Wconversion
#        -Wctor-dtor-privacy
#        -Wenum-compare
#        -Wfloat-equal
#        -Wnon-virtual-dtor
#        -Wold-style-cast
#        -Woverloaded-virtual
#        -Wredundant-decls
#        -Wsign-conversion
#        -Wsign-promo
#    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/W4 /WX)
endif()

#if(NOT CMAKE_CXX_EXTENSIONS)
#    set(CMAKE_CXX_EXTENSIONS OFF)
#endif()
#find_package(Qt5Test REQUIRED)
#set(CMAKE_INCLUDE_CURRENT_DIR ON)
#set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


option(USE_LIBWSS "Use custom websocket server library" ON)
configure_file(wsslibConfig.h.in wsslibConfig.h)

#add_subdirectory(cwss)

if(USE_LIBWSS)
    add_subdirectory(cwss)
    list(APPEND EXTRA_LIBS cwss)
endif()

#target_link_libraries(${PROJECT_NAME}
#        "${EXTRA_LIBS}"
#    )

#target_include_directories(${PROJECT_NAME}
#    PUBLIC
#        ${PROJECT_BINARY_DIR}
#    )

find_package( Boost 1.78 COMPONENTS program_options REQUIRED )
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
include_directories( ${Boost_INCLUDE_DIR} )

add_executable(websocket client.cpp)

TARGET_LINK_LIBRARIES( websocket LINK_PUBLIC ${Boost_LIBRARIES} Threads::Threads ${Boost_LIBRARIES} boost_system boost_thread)



add_executable(${PROJECT_NAME}
#    EXCLUDE_FROM_ALL
    test-server.cpp
    )
target_link_libraries(${PROJECT_NAME}
#    PUBLIC
        "${EXTRA_LIBS}"
#    PRIVATE
#        Qt5::Test
        )
#target_link_libraries(test2 PRIVATE Qt5::Test)
#add_executable(test1 test1.cpp)


#get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
#foreach(dir ${dirs})
#  message(STATUS "dir='${dir}'")
#endforeach()


set(CTEST_PARALLEL_LEVEL 2)

enable_testing()
set(MULTIPLY_TIMEOUT 3)

add_test(Usage wssserver 1)
add_test(Runs wssserver "0.0.0.0" 43211 1)
add_test(Out1 websocket "127.0.0.1" 43211)
add_test(ClientTest websocket )

set_tests_properties(Runs Usage Out1
    PROPERTIES
        TIMEOUT ${MULTIPLY_TIMEOUT}
    )

set_tests_properties(Usage ClientTest
    PROPERTIES
        PASS_REGULAR_EXPRESSION "Usage"
    )


set_tests_properties(Out1
    PROPERTIES
    PASS_REGULAR_EXPRESSION "client"
    FAIL_REGULAR_EXPRESSION "Error"
)



# define a function to simplify adding tests
#function(do_test target arg result)
#  add_test(NAME Comp${arg} COMMAND ${target} ${arg})
#  set_tests_properties(Comp${arg}
#    PROPERTIES PASS_REGULAR_EXPRESSION ${result}
#    )
#endfunction()

#do_test(wssserver 4 0)






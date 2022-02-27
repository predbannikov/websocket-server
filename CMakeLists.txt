cmake_minimum_required(VERSION 3.5)
project(wssserver VERSION 1.0 LANGUAGES CXX)
get_directory_property(IS_SUBPROJECT PARENT_DIRECTORY)

###################################################################################################
##
##      Опции проекта
##
###################################################################################################

if (IS_SUBPROJECT AND NOT DEFINED MYLIB_TESTING)
    message(STATUS "Mylib не тестируется в режиме подмодуля")
    set(MYLIB_TESTING OFF)
endif()

if (IS_SUBPROJECT AND NOT DEFINED MYLIB_DOC)
    message(STATUS "Автодокументация для Mylib выключена в режиме подмодуля")
    set(MYLIB_DOC OFF)
endif()

option(MYLIB_TESTING "Включить модульное тестирование" ON)
option(MYLIB_COVERAGE "Включить измерение покрытия кода тестами" OFF)
option(MYLIB_DOC "Включить документирование" ON)

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

if(NOT CMAKE_CXX_EXTENSIONS)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(Doxygen)
if (Doxygen_FOUND)
    if (NOT MYLIB_DOXYGEN_LANGUAGE)
        set(MYLIB_DOXYGEN_LANGUAGE Russian)
    endif()
    message(STATUS "Doxygen documentation will be generated in ${MYLIB_DOXYGEN_LANGUAGE}")
    configure_file(Doxyfile.in Doxyfile)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/../docs/Doxyfile.in)
    add_custom_target(doc ALL COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    )

endif ()

file(GLOB_RECURSE LibFiles "include/*.hpp")
add_custom_target(headers SOURCES ${LibFiles})


option(USE_LIBWSS "Use custom websocket server library" ON)
configure_file(wssserverConfig.h.in wssserverConfig.h)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    )
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package( Boost 1.78 COMPONENTS program_options REQUIRED )
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
include_directories( ${Boost_INCLUDE_DIR} )

add_executable(client client.cpp)
TARGET_LINK_LIBRARIES(client
    PRIVATE ${Boost_LIBRARIES} Threads::Threads ${Boost_LIBRARIES} boost_system boost_thread
)

add_executable(run-server test-server.cpp )
target_link_libraries(run-server
    PRIVATE ${Boost_LIBRARIES} Threads::Threads ${Boost_LIBRARIES} boost_system boost_thread
)


#get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
#foreach(dir ${dirs})
#  message(STATUS "dir='${dir}'")
#endforeach()
set(CTEST_PARALLEL_LEVEL 2)

enable_testing()
set(MULTIPLY_TIMEOUT 3)

add_test(Usage run-server 1)
add_test(Runs run-server "0.0.0.0" 43211 1)
add_test(Out1 client "127.0.0.1" 43211)
add_test(ClientTest client )

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






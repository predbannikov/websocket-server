 
add_library(libwss wss.h wss.cpp)
target_include_directories(libwss
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
    )

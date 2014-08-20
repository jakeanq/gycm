cmake_minimum_required (VERSION 2.8.1)

project(GYCM)

set (CMAKE_EXPORT_COMPILE_COMMANDS 1) # For ycmd completing

include(FindPkgConfig)

pkg_search_module(GEANY REQUIRED geany)

include_directories(
	${GEANY_INCLUDE_DIRS}
)

add_library(gycm
	gycm.cpp
)

target_link_libraries(gycm
	${GEANY_LIBRARIES}
)

if(MSVC)
        if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
                string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        else()
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
        endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR
                CMAKE_CXX_COMPILER MATCHES "clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -O2 --std=c++11")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -std=c99 -O2") 
        string(REGEX REPLACE " -g " " -ggdb " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
else()
        MESSAGE("Probably shouldn't be here; what compiler are you using?")
endif()

if(CMAKE_CXX_COMPILER MATCHES "clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
endif()

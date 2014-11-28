cmake_minimum_required (VERSION 2.8.1)

project(GYCM)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1) # For ycmd completing
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(FindPkgConfig)

pkg_search_module(GEANY REQUIRED geany)
find_package(Neon REQUIRED) # HTTP client
find_package(JsonCpp REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(
	${GEANY_INCLUDE_DIRS}
	${NEON_INCLUDE_DIRS}
	${OPENSSL_INCLUDE_DIRS}
	${JSONCPP_INCLUDE_DIRS}
)

add_library(gycm SHARED
	gycm.cpp
	ycmd.cpp
	utils.cpp
)
message(${GEANY_LIBRARIES})

target_link_libraries(gycm
	${GEANY_LIBRARIES}
	${NEON_LIBRARIES}
	${OPENSSL_LIBRARIES}
	${JSONCPP_LIBRARIES}
)

set_target_properties(gycm PROPERTIES PREFIX "")

string(REGEX REPLACE ";" " " GEANY_CFLAGS_OTHER "${GEANY_CFLAGS_OTHER}") # Not really sure how/why semicolons got into the output...

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GEANY_CFLAGS_OTHER}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GEANY_CFLAGS_OTHER}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${GEANY_LDFLAGS_OTHER}")

if(MSVC)
        if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
                string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        else()
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
        endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER MATCHES "clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -O2 --std=c++11 -Wno-ignored-attributes -Wno-missing-field-initializers")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -std=c99 -O2 -Wno-ignored-attributes -Wno-missing-field-initializers") 
        string(REGEX REPLACE " -g " " -ggdb " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
else()
        message("Probably shouldn't be here; what compiler are you using?")
endif()

if(CMAKE_CXX_COMPILER MATCHES "clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
endif()

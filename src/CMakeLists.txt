cmake_minimum_required(VERSION 3.5)
project(mod_voicechanger)

# Set FreeSWITCH paths
set(FREESWITCH_INCLUDE_DIRS "/usr/local/freeswitch/include/freeswitch")
set(FREESWITCH_LIBRARIES "/usr/local/freeswitch/lib/libfreeswitch.so")
set(FREESWITCH_MODULES_DIR "/usr/local/freeswitch/mod")

# Include directories
include_directories(
    ${FREESWITCH_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/signalsmith-stretch
    ${CMAKE_CURRENT_SOURCE_DIR}/signalsmith-stretch/dsp
    ${CMAKE_CURRENT_SOURCE_DIR}/../linear 
)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -std=c++11")

# Source files
set(SOURCES mod_voicechanger.cpp)

# Build target
add_library(mod_voicechanger MODULE ${SOURCES})
target_link_libraries(mod_voicechanger ${FREESWITCH_LIBRARIES})
set_target_properties(mod_voicechanger PROPERTIES PREFIX "")

# Installation
install(TARGETS mod_voicechanger
    DESTINATION ${FREESWITCH_MODULES_DIR}
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                GROUP_READ GROUP_EXECUTE 
                WORLD_READ WORLD_EXECUTE
)
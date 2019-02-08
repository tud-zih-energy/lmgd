cmake_minimum_required(VERSION 3.9)
project(lmgd)

include(cmake/DefaultBuildType.cmake)
include(cmake/GitSubmoduleUpdate.cmake)

git_submodule_update()

option(BUILD_SCOREP_PLUGIN "Include the Score-p metric plugin in the build." OFF)

add_subdirectory(lib)

set(SOURCE_FILES
    src/main.cpp

    src/network/network_socket.cpp
    src/network/serial_socket.cpp
    src/network/connection.cpp

    src/source/source.cpp
    src/source/log.cpp

    src/device/track.cpp
    src/device/device.cpp
    src/device/channel.cpp
)

add_executable(lmgd ${SOURCE_FILES})
target_compile_features(lmgd PUBLIC cxx_std_17)
target_link_libraries(lmgd
    PRIVATE
        asio
        Nitro::log
        Nitro::jiffy
        metricq::source
        json::json
)
target_include_directories(lmgd PUBLIC include)
target_compile_options(lmgd PUBLIC $<$<CONFIG:Debug>:-Wall -pedantic -Wextra>)

install(TARGETS lmgd
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

add_executable(ilmg src/interactive.cpp src/network/serial_socket.cpp src/network/network_socket.cpp src/network/connection.cpp)
target_compile_features(ilmg PUBLIC cxx_std_17)
target_link_libraries(ilmg PRIVATE asio pthread Nitro::log Nitro::jiffy)
target_include_directories(ilmg PUBLIC include)

if(BUILD_SCOREP_PLUGIN)
    add_library(lmg_plugin MODULE src/module.cpp src/network/socket.cpp)
    target_compile_features(lmg_plugin PUBLIC cxx_std_17)
    target_link_libraries(lmg_plugin
        PRIVATE
            Scorep::scorep-plugin-cxx
            asio
            Nitro::jiffy
    )
    target_include_directories(lmg_plugin PUBLIC include)
endif()

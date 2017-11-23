cmake_minimum_required(VERSION 3.9)
project(lmgd)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)

    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
            "MinSizeRel" "RelWithDebInfo")
endif()

# Intialize git submodules if not done already
file(GLOB SUBMODULE_FILES "lib/asio/*")
list(LENGTH SUBMODULE_FILES COUNT_ASIO)
file(GLOB SUBMODULE_FILES "lib/nitro/*")
list(LENGTH SUBMODULE_FILES COUNT_NITRO)
if(${COUNT_ASIO} EQUAL 0 OR ${COUNT_NITRO} EQUAL 0)
    message(STATUS "Initializing git submodule")
    execute_process(COMMAND "git" "submodule" "init" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    execute_process(COMMAND "git" "submodule" "update" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()


add_subdirectory(lib)

set(SOURCE_FILES src/main.cpp
                 src/socket.cpp)

add_executable(lmgd ${SOURCE_FILES})
target_compile_features(lmgd PUBLIC cxx_std_17)
target_link_libraries(lmgd PRIVATE asio Nitro::Log Nitro::Jiffy)
target_include_directories(lmgd PUBLIC include)

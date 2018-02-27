
add_library(asio INTERFACE)
target_include_directories(asio SYSTEM INTERFACE asio/asio/include)


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(asio INTERFACE Threads::Threads)

set(NITRO_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(nitro)

if(BUILD_SCOREP_PLUGIN)
    add_subdirectory(scorep_plugin_cxx_wrapper)
endif()

add_subdirectory(dataheap2)

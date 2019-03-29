set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(BUILD_SCOREP_PLUGIN)
    add_subdirectory(scorep_plugin_cxx_wrapper)
endif()

add_subdirectory(metricq)


add_library(asio INTERFACE)
target_include_directories(asio SYSTEM INTERFACE asio/asio/include)


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(asio INTERFACE Threads::Threads)

add_subdirectory(nitro)

# Clap your hands, when it's hacky and you know it.
# CLAP CLAP CLAP CLAP CLAP CLAP CLAP CLAP CLAP CLAP CLAP 
add_library(ScorepPluginCXX INTERFACE)
target_include_directories(ScorepPluginCXX SYSTEM INTERFACE scorep_plugin_cxx_wrapper/include)
target_link_libraries(ScorepPluginCXX INTERFACE Nitro::Log)

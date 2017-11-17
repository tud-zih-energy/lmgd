
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE asio/asio/include)


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(asio INTERFACE Threads::Threads)


add_subdirectory(nitro)

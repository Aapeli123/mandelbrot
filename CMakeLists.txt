cmake_minimum_required(VERSION 3.16)
project(mandelbrot)

find_package(SDL2 REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(mandelbrot ${SDL2_INCLUDE_DIRS})
add_executable(mandelbrot main.cpp)

target_link_libraries(mandelbrot ${SDL2_LIBRARIES} Threads::Threads)
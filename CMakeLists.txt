cmake_minimum_required(VERSION 3.10)
project(TaskManager CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_library(task_core
    src/Task.cpp
    src/TaskManager.cpp
)

add_executable(task_manager src/main.cpp)
target_link_libraries(task_manager PRIVATE task_core)

add_executable(task_web src/server.cpp)
target_link_libraries(task_web PRIVATE task_core Threads::Threads)

if(WIN32)
    target_link_libraries(task_web PRIVATE ws2_32)
endif()

if(MSVC)
    target_compile_options(task_core PRIVATE /W4)
    target_compile_options(task_manager PRIVATE /W4)
    target_compile_options(task_web PRIVATE /utf-8)
else()
    target_compile_options(task_core PRIVATE -Wall -Wextra)
    target_compile_options(task_manager PRIVATE -Wall -Wextra)
endif()

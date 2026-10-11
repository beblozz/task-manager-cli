cmake_minimum_required(VERSION 3.14)
project(TaskManager VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(TASK_BUILD_TESTS "Build unit tests" ON)

find_package(Threads REQUIRED)

# Общая логика — отдельная библиотека. Её используют консоль, сервер и тесты.
add_library(task_core
    src/Task.cpp
    src/TaskManager.cpp
    src/Utils.cpp
)
target_include_directories(task_core PUBLIC src)

add_executable(task_manager src/main.cpp)
target_link_libraries(task_manager PRIVATE task_core)

add_executable(task_web src/server.cpp)
target_link_libraries(task_web PRIVATE task_core Threads::Threads)

if(WIN32)
    # ws2_32 — сетевая библиотека Windows.
    # cpp-httplib требует Windows 10+, старый MinGW по умолчанию считает иначе
    target_link_libraries(task_web PRIVATE ws2_32)
    target_compile_definitions(task_web PRIVATE _WIN32_WINNT=0x0A00)
endif()

# Предупреждения компилятора. /utf-8 нужен MSVC, чтобы русские комментарии
# читались правильно.
if(MSVC)
    foreach(target task_core task_manager task_web)
        target_compile_options(${target} PRIVATE /utf-8)
    endforeach()
    target_compile_options(task_core PRIVATE /W4)
    target_compile_options(task_manager PRIVATE /W4)
else()
    target_compile_options(task_core PRIVATE -Wall -Wextra)
    target_compile_options(task_manager PRIVATE -Wall -Wextra)
endif()

if(TASK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

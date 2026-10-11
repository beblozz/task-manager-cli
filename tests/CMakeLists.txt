# GoogleTest скачивается автоматически при первой сборке (нужен интернет)
include(FetchContent)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
# На Windows (MSVC) иначе будет конфликт версий стандартной библиотеки
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(task_tests
    test_task.cpp
    test_task_manager.cpp
    test_storage.cpp
    test_utils.cpp
)
target_link_libraries(task_tests PRIVATE task_core GTest::gtest_main)

if(MSVC)
    target_compile_options(task_tests PRIVATE /utf-8)
endif()

# Регистрируем каждый TEST() отдельно, чтобы ctest показывал их по именам
include(GoogleTest)
gtest_discover_tests(task_tests)

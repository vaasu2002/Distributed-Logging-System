cmake_minimum_required(VERSION 3.20)
project(CentralLoggingSystem LANGUAGES CXX)

# Recommended defaults
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Library
add_library(mylib
    src/main.cpp
)
target_include_directories(mylib PUBLIC include)

# Executable
add_executable(myapp app/main.cpp)
target_link_libraries(myapp PRIVATE mylib)

# Enable testing
include(CTest)
if(BUILD_TESTING)
    add_executable(test_hello tests/test_hello.cpp)
    target_link_libraries(test_hello PRIVATE mylib)
    add_test(NAME hello_runs COMMAND test_hello)
endif()

cmake_minimum_required(VERSION 3.15)
project(OceanMini LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG master#v1.2.0
)

FetchContent_MakeAvailable(Crow)

add_executable(oceanmini oceanmini.cpp)

target_compile_definitions(oceanmini PRIVATE
    ASIO_ENABLE_OLD_SERVICES
)

target_link_libraries(oceanmini PRIVATE Crow::Crow)
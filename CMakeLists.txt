cmake_minimum_required(VERSION 3.25)
project(EasySteer VERSION 1.0.0 LANGUAGES CXX)

option(AFC_BUILD_PLUGIN "Build the Windows SKSE DLL" ON)
option(AFC_BUILD_TESTS "Build standalone behavior tests" ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(AutoFollowCore STATIC src/FollowController.cpp src/Settings.cpp)
target_include_directories(AutoFollowCore PUBLIC include)
if(MSVC)
    target_compile_options(AutoFollowCore PRIVATE /W4 /permissive-)
else()
    target_compile_options(AutoFollowCore PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(AFC_BUILD_TESTS)
    enable_testing()
    add_executable(AutoFollowTests tests/CoreTests.cpp)
    target_link_libraries(AutoFollowTests PRIVATE AutoFollowCore)
    add_test(NAME AutoFollowBehavior COMMAND AutoFollowTests)
endif()

if(AFC_BUILD_PLUGIN)
    if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "The SKSE plugin requires Windows x64 and MSVC. Use -DAFC_BUILD_PLUGIN=OFF for portable tests.")
    endif()
    include(FetchContent)
    set(BUILD_TESTS OFF CACHE BOOL "CommonLib tests" FORCE)
    set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
    set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
    set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(CommonLibSSE
        GIT_REPOSITORY https://github.com/alandtse/CommonLibSSE-NG.git
        GIT_TAG a898f469851c464d05137bb74b069dd234897643)
    FetchContent_MakeAvailable(CommonLibSSE)
    include(${commonlibsse_SOURCE_DIR}/cmake/CommonLibSSE.cmake)
    add_commonlibsse_plugin(EasySteer
        NAME "Easy Steer"
        AUTHOR "mlw05"
        USE_ADDRESS_LIBRARY
        SOURCES src/Plugin.cpp src/CameraHooks.cpp)
    target_include_directories(EasySteer PRIVATE include)
    target_link_libraries(EasySteer PRIVATE AutoFollowCore)
    target_precompile_headers(EasySteer PRIVATE include/AutoFollow/PCH.h)
    install(TARGETS EasySteer RUNTIME DESTINATION SKSE/Plugins COMPONENT EasySteer)
    install(FILES config/EasySteer.ini DESTINATION SKSE/Plugins COMPONENT EasySteer)
    install(FILES README.md docs/CHANGELOG.md docs/TESTING.md DESTINATION docs/EasySteer COMPONENT EasySteer)
    install(DIRECTORY docs/licenses DESTINATION docs/EasySteer COMPONENT EasySteer)
endif()

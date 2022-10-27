cmake_minimum_required(VERSION 3.10.2)

PROJECT(quest-demo C CXX)

add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

file(GLOB_RECURSE SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp/*.h)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/vendor")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/vendor/ovr_sdk_mobile/VrApi/Include")

add_library(quest-demo SHARED ${SOURCE_FILES})

target_include_directories(quest-demo PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue)

add_library(ovr-loader SHARED IMPORTED)
set_target_properties(ovr-loader PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/vendor/ovr_sdk_mobile/VrApi/Libs/Android/arm64-v8a/Release/libvrapi.so)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

target_link_libraries(
        quest-demo
        ovr-loader
        android
        native_app_glue
        log
        GLESv3
        EGL)

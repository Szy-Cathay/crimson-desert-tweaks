cmake_minimum_required(VERSION 3.20)

project(crimson-desert-tweaks LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Local Zydis for offline builds (same mechanism as player-status-modifier) ──

set(CRIMSON_DESERT_TWEAKS_LOCAL_ZYDIS_DIR "" CACHE PATH "Optional local Zydis v4.1.0 source directory for offline builds")
if(CRIMSON_DESERT_TWEAKS_LOCAL_ZYDIS_DIR)
    file(TO_CMAKE_PATH "${CRIMSON_DESERT_TWEAKS_LOCAL_ZYDIS_DIR}" CRIMSON_DESERT_TWEAKS_LOCAL_ZYDIS_DIR_CMAKE)
    if(EXISTS "${CRIMSON_DESERT_TWEAKS_LOCAL_ZYDIS_DIR_CMAKE}/CMakeLists.txt")
        set(FETCHCONTENT_SOURCE_DIR_ZYDIS "${CRIMSON_DESERT_TWEAKS_LOCAL_ZYDIS_DIR_CMAKE}" CACHE PATH "Local Zydis source used by SafetyHook" FORCE)
        message(STATUS "Using local Zydis source: ${CRIMSON_DESERT_TWEAKS_LOCAL_ZYDIS_DIR_CMAKE}")
    else()
        message(WARNING "CRIMSON_DESERT_TWEAKS_LOCAL_ZYDIS_DIR does not contain CMakeLists.txt: ${CRIMSON_DESERT_TWEAKS_LOCAL_ZYDIS_DIR_CMAKE}")
    endif()
endif()

set(CRIMSON_DESERT_TWEAKS_LOCAL_ZYCORE_DIR "" CACHE PATH "Optional local Zycore source directory for offline Zydis builds")
if(CRIMSON_DESERT_TWEAKS_LOCAL_ZYCORE_DIR)
    file(TO_CMAKE_PATH "${CRIMSON_DESERT_TWEAKS_LOCAL_ZYCORE_DIR}" CRIMSON_DESERT_TWEAKS_LOCAL_ZYCORE_DIR_CMAKE)
    if(EXISTS "${CRIMSON_DESERT_TWEAKS_LOCAL_ZYCORE_DIR_CMAKE}/CMakeLists.txt")
        set(ZYAN_ZYCORE_PATH "${CRIMSON_DESERT_TWEAKS_LOCAL_ZYCORE_DIR_CMAKE}" CACHE PATH "Local Zycore source used by Zydis" FORCE)
        set(ZYDIS_ZYCORE_PATH "${CRIMSON_DESERT_TWEAKS_LOCAL_ZYCORE_DIR_CMAKE}" CACHE PATH "Local Zycore source used by Zydis" FORCE)
        message(STATUS "Using local Zycore source: ${CRIMSON_DESERT_TWEAKS_LOCAL_ZYCORE_DIR_CMAKE}")
    else()
        message(WARNING "CRIMSON_DESERT_TWEAKS_LOCAL_ZYCORE_DIR does not contain CMakeLists.txt: ${CRIMSON_DESERT_TWEAKS_LOCAL_ZYCORE_DIR_CMAKE}")
    endif()
endif()

# ── SafetyHook ─────────────────────────────────────────────────────────

set(SAFETYHOOK_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(SAFETYHOOK_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(SAFETYHOOK_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/safetyhook)

# ── Main ASI target ────────────────────────────────────────────────────

add_library(crimson-desert-tweaks SHARED
    src/config.cpp
    src/config_watcher.cpp
    src/dllmain.cpp
    src/hooks/shield_hide.cpp
    src/logger.cpp
    src/scanner.cpp
    src/version.rc
)

target_include_directories(crimson-desert-tweaks PRIVATE src)
target_link_libraries(crimson-desert-tweaks PRIVATE safetyhook)

target_compile_definitions(crimson-desert-tweaks PRIVATE
    NOMINMAX
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    CRIMSON_DESERT_TWEAKS_BUILD_ID="v1.0.0-shield-hide"
)

if(MSVC)
    target_compile_options(crimson-desert-tweaks PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/utf-8>
        $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
        $<$<COMPILE_LANGUAGE:CXX>:/W4>
        $<$<COMPILE_LANGUAGE:CXX>:/permissive->
    )
endif()

set_target_properties(crimson-desert-tweaks PROPERTIES
    OUTPUT_NAME "crimson-desert-tweaks"
    SUFFIX ".asi"
)

# ── Copy output to mod directory ───────────────────────────────────────

set(CRIMSON_DESERT_TWEAKS_OUTPUT_DIR "D:/红色沙漠工具/模组/Crimson Desert Tweaks" CACHE PATH "Output directory for .asi and default .ini")

add_custom_command(TARGET crimson-desert-tweaks POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CRIMSON_DESERT_TWEAKS_OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:crimson-desert-tweaks>"
        "${CRIMSON_DESERT_TWEAKS_OUTPUT_DIR}/$<TARGET_FILE_NAME:crimson-desert-tweaks>"
    COMMENT "Copying .asi to output directory..."
)

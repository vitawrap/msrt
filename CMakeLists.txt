cmake_minimum_required(VERSION 3.10)
project(msrt LANGUAGES C CXX ASM)
# force older dwarf debug info
add_compile_options(-gdwarf-4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# specifically to help raylib build
cmake_policy(SET CMP0048 NEW) 
cmake_policy(SET CMP0077 NEW)

# find gtk for linux message boxes (or use win32 api on windows)
if (UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK IMPORTED_TARGET gtk+-3.0)
    if (GTK_FOUND)
        set(MESSAGEBOX_LIB PkgConfig::GTK)
        add_compile_definitions(PLATFORM_UNIX_MESSAGEBOXES)
    endif()
elseif(WIN32)
    set(ZLIB_LIBRARY $ENV{ZLIB_LIBRARY})
    set(ZLIB_INCLUDE_DIR $ENV{ZLIB_INCLUDE_DIR})
    add_compile_definitions(PLATFORM_NT_MESSAGEBOXES)
endif()

include(FetchContent)

# disable examples and tests to build faster
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG        5.5    # 5.5 for compatibility with cmake
)

FetchContent_Declare(
  quickjs
  GIT_REPOSITORY https://github.com/bellard/quickjs.git
  GIT_TAG        d7ae12ae71dfd6ab2997527d295014a8996fa0f9 # master as of april 2026
)

FetchContent_Declare(
  minizip
  GIT_REPOSITORY https://github.com/domoticz/minizip.git
  GIT_TAG        aee7fbddf118d9363575af96310c29fa747d70c0 # master as of april 2026
)

# this is horrible, yet necessary for cjson and raylib to work
macro(add_custom_target _target)
    if(NOT "${_target}" STREQUAL "uninstall")
        _add_custom_target(${ARGV})
    endif()
endmacro()

FetchContent_MakeAvailable(minizip raylib quickjs)

# quickjs needs manual setup
set(QJS_LIB ${quickjs_SOURCE_DIR}/libquickjs.a)
add_custom_command(OUTPUT ${QJS_LIB}
    COMMAND make
    WORKING_DIRECTORY ${quickjs_SOURCE_DIR}
    COMMENT "Building QuickJS from Makefile..."
)
add_custom_target(quickjs_build DEPENDS ${QJS_LIB})

file(GLOB_RECURSE SOURCES 
    "src/*.cpp" 
    "src/*.c"
)

file(GLOB_RECURSE SOURCES_ASM
    "src/*.s"
)

set_property(SOURCE ${SOURCES_ASM} APPEND PROPERTY COMPILE_OPTIONS "-I${CMAKE_SOURCE_DIR}")

# tell cmake that ASM sources depend on JS scripts

file(GLOB_RECURSE SOURCES_JS
    "js_scripts/*.js"
)

set_source_files_properties(${SOURCES_ASM}
    PROPERTIES OBJECT_DEPENDS "${SOURCES_JS}"
)

# main executable

add_executable(${PROJECT_NAME} ${SOURCES} ${SOURCES_ASM})
target_include_directories(${PROJECT_NAME} PRIVATE src include)
target_include_directories(${PROJECT_NAME} PUBLIC "${quickjs_SOURCE_DIR}" "${minizip_SOURCE_DIR}")

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        raylib
        minizip
        ${QJS_LIB}
        ${MESSAGEBOX_LIB}
)

# link-time linux specific shenanigans
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE m pthread dl rt)
endif()

# test executable

file(GLOB_RECURSE TEST_SOURCES
    "tests/*.cpp"
)

add_executable(unit_tests ${TEST_SOURCES} ${SOURCES} ${SOURCES_ASM})
target_compile_definitions(unit_tests PRIVATE PLATFORM_NO_MAIN)
target_include_directories(unit_tests PRIVATE src include)
target_include_directories(unit_tests PUBLIC "${quickjs_SOURCE_DIR}" "${minizip_SOURCE_DIR}")

target_link_libraries(unit_tests 
    PRIVATE 
        raylib
        minizip
        ${QJS_LIB}
        ${MESSAGEBOX_LIB}
)

# link-time linux specific shenanigans
if(UNIX AND NOT APPLE)
    target_link_libraries(unit_tests PRIVATE m pthread dl rt)
endif()

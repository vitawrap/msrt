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

# release config
add_compile_options(
    "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-O3>"
    "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-ffast-math>"
    "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-fno-stack-protector>"
    #"$<$<CONFIG:Debug>:-fsanitize=address>"
    #"$<$<CONFIG:Debug>:-O1>"
    #"$<$<CONFIG:Debug>:-fno-omit-frame-pointer>"
)

add_link_options(
    #"$<$<CONFIG:Debug>:-fsanitize=address>"
)

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
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
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
  GIT_REPOSITORY https://github.com/quickjs-ng/quickjs.git
  GIT_TAG        v0.14.0 # latest release: april 2026
)

FetchContent_Declare(
  minizip
  GIT_REPOSITORY https://github.com/domoticz/minizip.git
  GIT_TAG        aee7fbddf118d9363575af96310c29fa747d70c0 # master as of april 2026
)

# for snmalloc, force it to override os allocator symbols, this is apparently risky on windows
set(SNMALLOC_STATIC_LIBRARY_PREFIX "" CACHE STRING "Remove prefix to override global allocator")

FetchContent_Declare(
  snmalloc
  GIT_REPOSITORY https://github.com/microsoft/snmalloc.git
  GIT_TAG        48fd18a3c9baaeb89205c654e12765ff3c5ff902 # master as of may 2026
)


# this is horrible, yet necessary for cjson and raylib to work
macro(add_custom_target _target)
    if(NOT "${_target}" STREQUAL "uninstall")
        _add_custom_target(${ARGV})
    endif()
endmacro()

FetchContent_MakeAvailable(minizip raylib quickjs snmalloc)

# quickjs needs manual setup
# set(QJS_LIB ${quickjs_SOURCE_DIR}/libquickjs.a)
# add_custom_command(OUTPUT ${QJS_LIB}
#     COMMAND make
#     WORKING_DIRECTORY ${quickjs_SOURCE_DIR}
#     COMMENT "Building QuickJS from Makefile..."
# )
# add_custom_target(quickjs_build DEPENDS ${QJS_LIB})

file(GLOB_RECURSE SOURCES 
    "src/*.cpp" 
    "src/*.c"
)

file(GLOB_RECURSE SOURCES_ASM
    "src/*.S"
)

set_property(SOURCE ${SOURCES_ASM} APPEND PROPERTY COMPILE_OPTIONS "-I${CMAKE_SOURCE_DIR}")

# tell cmake that ASM sources depend on assets/JS scripts

file(GLOB_RECURSE BIN_ASSETS
    "assets/*"
)

set_source_files_properties(${SOURCES_ASM}
    PROPERTIES OBJECT_DEPENDS "${BIN_ASSETS}"
)

# main executable

add_executable(${PROJECT_NAME} ${SOURCES} ${SOURCES_ASM})
target_include_directories(${PROJECT_NAME} PRIVATE src include)
target_include_directories(${PROJECT_NAME} PUBLIC "${ZLIB_INCLUDE_DIR}" "${quickjs_SOURCE_DIR}" "${minizip_SOURCE_DIR}")

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        snmallocshim-static
        raylib
        minizip
        qjs
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
target_include_directories(unit_tests PUBLIC "${ZLIB_INCLUDE_DIR}" "${quickjs_SOURCE_DIR}" "${minizip_SOURCE_DIR}")

target_link_libraries(unit_tests 
    PRIVATE 
        snmallocshim-static
        raylib
        minizip
        qjs
        ${MESSAGEBOX_LIB}
)

# link-time linux specific shenanigans
if(UNIX AND NOT APPLE)
    target_link_libraries(unit_tests PRIVATE m pthread dl rt)
endif()

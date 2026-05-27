cmake_minimum_required(VERSION 3.10)
project(msvm LANGUAGES C CXX ASM)
# force older dwarf debug info
add_compile_options(-gdwarf-4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED 3.8.2)

file(GLOB_RECURSE SOURCES 
    "src/*.cpp" 
    "src/*.c"
)

# Fill flex/bison variables
set(FLEX_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/ast/lexer.l)
set(BISON_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/ast/parser.y)
set(LEXER_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/ast/lexer.h)
set(PARSER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/ast/parser.cc)

# Set up flex/bison
flex_target(Lexer ${FLEX_INPUT} ${LEXER_HEADER})
bison_target(Parser ${BISON_INPUT} ${PARSER_SOURCE} COMPILE_FLAGS "-Wcounterexamples")

add_library(${PROJECT_NAME} STATIC ${FLEX_Lexer_OUTPUTS} ${BISON_Parser_OUTPUTS} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src ../../include)

# link-time linux specific shenanigans
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE m pthread rt)
endif()

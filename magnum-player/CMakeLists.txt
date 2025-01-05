cmake_minimum_required(VERSION 3.13)
project(magnum-player)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/main.cpp
    src/comm.cpp
    src/rt.cpp
    src/slide.cpp
)

# Add executable
add_executable(magnum-player ${SOURCES})

# Include directories for the project
include_directories(${CMAKE_SOURCE_DIR}/include)

# Find SDL2
find_package(SDL2 REQUIRED)
target_include_directories(magnum-player PRIVATE ${SDL2_INCLUDE_DIRS})

# Find SDL2_ttf
find_package(SDL2_ttf REQUIRED)
target_include_directories(magnum-player PRIVATE ${SDL2_TTF_INCLUDE_DIRS})

# Find FFmpeg libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale)
target_include_directories(magnum-player PRIVATE ${FFMPEG_INCLUDE_DIRS})

# Asset generation with Python script
find_package(Python3 REQUIRED)
set(INPUT_FONT_ASSET ${CMAKE_SOURCE_DIR}/KarmaSuture.ttf)
set(OUTPUT_FONT_HEADER ${CMAKE_SOURCE_DIR}/include/asset_Font.hpp)
set(PYTHON_SCRIPT ${CMAKE_SOURCE_DIR}/asset_gen.py)

add_custom_command(
    OUTPUT ${OUTPUT_FONT_HEADER}
    COMMAND Python3::Interpreter ${PYTHON_SCRIPT} ${INPUT_FONT_ASSET} ${OUTPUT_FONT_HEADER} FontData
    COMMENT "Generating asset buffer ${INPUT_FONT_ASSET} to ${OUTPUT_FONT_HEADER}"
    DEPENDS ${INPUT_FONT_ASSET} ${PYTHON_SCRIPT}
)

add_custom_target(GenerateAssetFont ALL DEPENDS ${OUTPUT_FONT_HEADER})
add_dependencies(magnum-player GenerateAssetFont)

# Link libraries
target_link_libraries(magnum-player PRIVATE ${FFMPEG_LIBRARIES})
target_link_libraries(magnum-player PRIVATE SDL2::SDL2 SDL2_ttf::SDL2_ttf)

message(STATUS "SDL2_TTF_INCLUDE_DIRS: ${SDL2_TTF_INCLUDE_DIRS}")
message(STATUS "SDL2_TTF_LIBRARIES: ${SDL2_TTF_LIBRARIES}")

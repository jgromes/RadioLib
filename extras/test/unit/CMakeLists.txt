cmake_minimum_required(VERSION 3.13)

project(radiolib-unittest)

# add RadioLib sources
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../../RadioLib" "${CMAKE_CURRENT_BINARY_DIR}/RadioLib")

# add test sources
file(GLOB_RECURSE TEST_SOURCES
  "tests/main.cpp"
  "tests/TestModule.cpp"
)

# create the executable
add_executable(${PROJECT_NAME} ${TEST_SOURCES})

# include directories
target_include_directories(${PROJECT_NAME} PUBLIC include)

# link RadioLib
target_link_libraries(${PROJECT_NAME} RadioLib fmt)

# set target properties and options
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)

# set RadioLib debug
#target_compile_definitions(RadioLib PUBLIC RADIOLIB_DEBUG_BASIC RADIOLIB_DEBUG_SPI)
#target_compile_definitions(RadioLib PUBLIC RADIOLIB_DEBUG_PORT=stdout)

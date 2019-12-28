cmake_minimum_required(VERSION 3.10)
project(hass-ds18b20)
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(FMT_HEADER_ONLY)

include_directories(src)
include_directories(third_party/raspi_lib)

file(GLOB_RECURSE SRC_GLOB_LIST src/*.cpp)
list(APPEND SRC_LIST ${SRC_GLOB_LIST})

add_executable(${PROJECT_NAME} ${SRC_LIST}
        third_party/raspi_lib/delay.c
        third_party/raspi_lib/gpio.c
        third_party/raspi_lib/ds18b20.c)

target_link_libraries(${PROJECT_NAME} mosquitto)


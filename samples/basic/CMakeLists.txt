# COPYRIGHT NOTICE: (c) 2022 Irnas. All rights reserved.

cmake_minimum_required(VERSION 3.13.1)

# create compile_commands.json for clang
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sentek-iot-probe-spi-mutex-test-nrf52)

zephyr_compile_options(-fdiagnostics-color=always)

zephyr_include_directories(src)
target_sources(app PRIVATE
  src/main.c
)

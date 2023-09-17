#!/bin/bash
cmake -B ./Build -S . -DCMAKE_TOOLCHAIN_FILE=arm-none-eabi-gcc.cmake

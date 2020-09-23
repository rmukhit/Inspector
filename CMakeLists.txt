cmake_minimum_required(VERSION 3.8)
project(P1_rmukhit)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        tests/01-compile-1.sh
        tests/02-print-1.sh
        tests/README.md
        tests/run_tests
        tests/test1.txt
        inspector.c
        README.md)

add_executable(P1_rmukhit ${SOURCE_FILES})
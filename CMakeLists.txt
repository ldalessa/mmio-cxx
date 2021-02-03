cmake_minimum_required(VERSION 3.18)
project(mmio-cxx LANGUAGES CXX C)

add_subdirectory(src)
add_subdirectory(examples)

add_library(mmio::mmio ALIAS mmio_lib)

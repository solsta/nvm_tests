cmake_minimum_required(VERSION 3.16)
project(Pmem_tests C)

set(CMAKE_C_STANDARD 99)

include_directories(../pmdk/src/include)
link_libraries(pmem pmemblk pmemlog pmemobj rpmem pmempool)


add_executable(Pmem_tests main.c)

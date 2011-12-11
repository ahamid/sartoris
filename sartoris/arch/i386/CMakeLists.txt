cmake_minimum_required (VERSION 2.6)

enable_language(ASM_NASM)

add_subdirectory(boot)
add_subdirectory(kernel)
add_subdirectory(lib)
add_subdirectory(sartoris)

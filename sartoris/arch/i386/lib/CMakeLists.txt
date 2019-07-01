cmake_minimum_required (VERSION 2.6)

# i don't know what this dir is for...

set(AS nasm)
set(AS_FLAGS -f elf -I ${CMAKE_CURRENT_SOURCE_DIR}/ -I ${CMAKE_CURRENT_BINARY_DIR}/)

add_custom_command(OUTPUT kserv.o COMMAND ${AS} ${AS_FLAGS} -o ${CMAKE_CURRENT_BINARY_DIR}/kserv.o kserv.s)
add_custom_command(OUTPUT kserv-user.o COMMAND ${AS} ${AS_FLAGS} -o ${CMAKE_CURRENT_BINARY_DIR}/kserv-user.o kserv-user.s)

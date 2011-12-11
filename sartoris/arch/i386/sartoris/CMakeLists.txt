cmake_minimum_required (VERSION 2.6)

set(AS nasm)
set(AS_FLAGS -f bin -I ${CMAKE_CURRENT_SOURCE_DIR}/ -I ${CMAKE_CURRENT_BINARY_DIR}/)

add_custom_target(sartoris_image ALL
  COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/../boot/bootsect ${CMAKE_CURRENT_BINARY_DIR}/sartoris.img
  COMMAND cat ${CMAKE_CURRENT_BINARY_DIR}/../kernel/kimg >> ${CMAKE_CURRENT_BINARY_DIR}/sartoris.img
  DEPENDS bootsect kimg
)

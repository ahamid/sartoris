cmake_minimum_required (VERSION 2.6)

set(AS nasm)
set(AS_FLAGS -f bin -I ${CMAKE_CURRENT_SOURCE_DIR}/ -I ${CMAKE_CURRENT_BINARY_DIR}/)

#execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/kernelblocks.pl ${CMAKE_CURRENT_BINARY_DIR}/../kernel/kimg OUTPUT_VARIABLE KERNBLOCKS)
add_custom_command(OUTPUT kern_sectors.inc COMMAND echo -n "%define kern_sectors " > ${CMAKE_CURRENT_BINARY_DIR}/kern_sectors.inc COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/kernelblocks.pl ${CMAKE_CURRENT_BINARY_DIR}/../kernel/kimg >> ${CMAKE_CURRENT_BINARY_DIR}/kern_sectors.inc)

add_custom_command(OUTPUT stage1 COMMAND ${AS} ${AS_FLAGS} -o ${CMAKE_CURRENT_BINARY_DIR}/stage1 -l ${CMAKE_CURRENT_BINARY_DIR}/stage1.map ${CMAKE_CURRENT_SOURCE_DIR}/stage1.s)
add_custom_command(OUTPUT stage1_5 COMMAND ${AS} ${AS_FLAGS} -o ${CMAKE_CURRENT_BINARY_DIR}/stage1_5 -l ${CMAKE_CURRENT_BINARY_DIR}/stage1_5.map ${CMAKE_CURRENT_SOURCE_DIR}/stage1_5.s)
add_custom_command(OUTPUT stage2 COMMAND ${AS} ${AS_FLAGS} -o ${CMAKE_CURRENT_BINARY_DIR}/stage2 -l ${CMAKE_CURRENT_BINARY_DIR}/stage2.map ${CMAKE_CURRENT_SOURCE_DIR}/stage2mem.s)
add_custom_command(OUTPUT sartoris_loader COMMAND ${AS} ${AS_FLAGS} -o ${CMAKE_CURRENT_BINARY_DIR}/sartoris_loader -l ${CMAKE_CURRENT_BINARY_DIR}/sartoris_loader.map ${CMAKE_CURRENT_SOURCE_DIR}/sartoris_loader.s DEPENDS kern_sectors.inc)
add_custom_target(bootsect ALL
  COMMAND rm -f bootsect
  COMMAND cat stage1 >> bootsect
  COMMAND cat stage1_5 >> bootsect
  COMMAND cat stage2 >> bootsect
  COMMAND cat sartoris_loader >> bootsect
  DEPENDS kernel_image stage1 stage1_5 stage2 sartoris_loader
)

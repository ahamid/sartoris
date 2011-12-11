cmake_minimum_required (VERSION 2.6)

set(CMAKE_ASM_NASM_OBJECT_FORMAT "bin")
set(CMAKE_ASM_NASM_FLAGS "-I ${CMAKE_CURRENT_SOURCE_DIR}/ -I ${CMAKE_CURRENT_BINARY_DIR}/")
# reset the nasm command because the output format gets hardcoded. are you freaking kidding me 8#
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <FLAGS> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

set(DST ${CMAKE_CURRENT_BINARY_DIR})

add_custom_command(OUTPUT kern_sectors.inc COMMAND echo -n "%define kern_sectors " > ${DST}/kern_sectors.inc COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/kernelblocks.pl ${DST}/../kernel/kimg >> ${DST}/kern_sectors.inc DEPENDS kimg)

# stupid hack to just get CMake to compile our files to objects...
add_library(bootsector_objects stage1.nasm stage1_5.nasm stage2mem.nasm sartoris_loader.nasm kern_sectors.inc)

add_custom_target(bootsect ALL
  COMMAND rm -f ${DST}/bootsect
  COMMAND cat stage1.nasm.o >> ${DST}/bootsect
  COMMAND cat stage1_5.nasm.o >> ${DST}/bootsect
  COMMAND cat stage2mem.nasm.o >> ${DST}/bootsect
  COMMAND cat sartoris_loader.nasm.o >> ${DST}/bootsect
  DEPENDS kimg bootsector_objects
  # another hack to get access to the generated object files.
  WORKING_DIRECTORY "${DST}/CMakeFiles/bootsector_objects.dir"
)

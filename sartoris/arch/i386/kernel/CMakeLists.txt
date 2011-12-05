cmake_minimum_required (VERSION 2.6)

set(FILL_UTIL "${PROJECT_SOURCE_DIR}/utils/fill")
set(SIZE_UTIL "${PROJECT_SOURCE_DIR}/arch/i386/size.sh")

set(cpu_objects main.c task.c pgrequest.c descriptor.c thread.c tss.c interrupt.c paging.c memory.c caps.c permissions.c ttrace.c)
set(prt_objects print.c)
set(assembly_basenames cpu-asm int pic stack_winding state_switch syscall-gates vga)

set(AS nasm)
set(AS_FLAGS -f elf -d FPU_MMX -D _METRICS_ -D PAGING -I ${CMAKE_CURRENT_SOURCE_DIR}/)
set(LINKER_FLAGS ${LD_FLAGS} -oformat=binary -Ttext 0x100000 -e 0x100000 -Map kernel.map)

# BLERGH! CMake assembly support actually really sucks :'(
set(assembly_objects)
foreach(obj ${assembly_basenames})
  add_custom_command(OUTPUT ${obj}.o COMMAND ${AS} ${AS_FLAGS} -o ${CMAKE_CURRENT_BINARY_DIR}/${obj}.o ${CMAKE_CURRENT_SOURCE_DIR}/${obj}.s)
  list(APPEND assembly_objects ${obj}.o)
endforeach(obj)

add_library(c_objects ${cpu_objects} ${prt_objects} ${assembly_objects})
set_target_properties(c_objects PROPERTIES COMPILE_FLAGS "-MD")

add_custom_target(kernel_image ALL COMMAND ld ${LINKER_FLAGS} ${CMAKE_CURRENT_BINARY_DIR}/libc_objects.a ${PROJECT_BINARY_DIR}/mk/libmicrokernel.a DEPENDS microkernel c_objects)

add_custom_command(TARGET kernel_image POST_BUILD COMMAND ${FILL_UTIL} libkimg.a -m 512 COMMAND ${SIZE_UTIL})

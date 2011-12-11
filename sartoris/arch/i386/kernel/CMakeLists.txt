cmake_minimum_required (VERSION 2.6)

set(FILL_UTIL "${PROJECT_SOURCE_DIR}/utils/fill")
set(SIZE_UTIL "${PROJECT_SOURCE_DIR}/arch/i386/size.sh")

set(cpu_objects main.c task.c pgrequest.c descriptor.c thread.c tss.c interrupt.c paging.c memory.c cpu-asm.nasm syscall-gates.nasm int.nasm pic.nasm state_switch.nasm stack_winding.nasm caps.c permissions.c ttrace.c)
set(prt_objects print.c vga.nasm)

set(CMAKE_ASM_NASM_FLAGS "-I ${CMAKE_CURRENT_SOURCE_DIR}/ -I ${CMAKE_CURRENT_BINARY_DIR}/")
set(LINKER_FLAGS ${LD_FLAGS} -oformat=binary -Ttext 0x100000 -e 0x100000 -Map kernel.map)

# build a simple static library first with CMake
add_library(objects STATIC ${cpu_objects} ${prt_objects})
# link the archive into the image with --whole-archive
add_custom_target(kimg ALL COMMAND ld ${LINKER_FLAGS} -o ${CMAKE_CURRENT_BINARY_DIR}/kimg ${PROJECT_BINARY_DIR}/mk/microkernel.so --whole-archive ${CMAKE_CURRENT_BINARY_DIR}/libobjects.a DEPENDS microkernel objects)

add_custom_command(TARGET kimg POST_BUILD COMMAND ${FILL_UTIL} kimg -m 512 COMMAND ${SIZE_UTIL})

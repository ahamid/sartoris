cmake_minimum_required (VERSION 2.6)

set(FILL_UTIL "${PROJECT_SOURCE_DIR}/utils/fill")
set(SIZE_UTIL "${PROJECT_SOURCE_DIR}/arch/i386/size.sh")

set(cpu_objects main.c task.c pgrequest.c descriptor.c thread.c tss.c interrupt.c paging.c memory.c caps.c permissions.c ttrace.c)
set(prt_objects print.c)
set(assembly_src cpu-asm.s arch/i386/kernel/int.s arch/i386/kernel/pic.s arch/i386/kernel/stack_winding.s arch/i386/kernel/state_switch.s arch/i386/kernel/syscall-gates.s arch/i386/kernel/vga.s)

#add_custom_command(OUTPUT assembly.o COMMAND ${AS} ${AS_FLAGS} -o assembly.o ${assembly_src})

#add_library(kimg ${cpu_objects} ${prt_objects} assembly.o)

set(LINKER_FLAGS "-oformat binary -Ttext 0x100000 -e 0x100000 -Map kernel.map")
set(LD_FLAGS ${LINKER_FLAGS})
set(LDFLAGS ${LINKER_FLAGS})
set(CMAKE_SHARED_LINKER_FLAGS  ${LINKER_FLAGS})
add_library(kimg ${cpu_objects} ${prt_objects})
set_target_properties(kimg PROPERTIES COMPILE_FLAGS "-MD" CMAKE_SHARED_LINKER_FLAGS ${LINKER_FLAGS})
target_link_libraries(kimg microkernel ${LINKER_FLAGS})

add_custom_command(TARGET kimg POST_BUILD COMMAND ${FILL_UTIL} libkimg.a -m 512 COMMAND ${SIZE_UTIL})

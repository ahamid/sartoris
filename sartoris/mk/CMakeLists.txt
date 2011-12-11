cmake_minimum_required (VERSION 2.6)

# create an archive
add_library(microkernel_archive STATIC main.c task.c thread.c paging.c interrupt.c metrics.c containers.c indexing.c salloc.c dynmem.c message.c shared-mem.c bitops.c error.c permissions.c ttrace.c)
# wrap it in a library via --whole-archive
add_custom_command(OUTPUT microkernel.so COMMAND ld ${LD_FLAGS} -i -o ${CMAKE_CURRENT_BINARY_DIR}/microkernel.so --whole-archive libmicrokernel_archive.a DEPENDS microkernel_archive) 
add_custom_target(microkernel DEPENDS microkernel.so)

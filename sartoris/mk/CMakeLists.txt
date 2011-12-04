cmake_minimum_required (VERSION 2.6)

# TODO: why is kimg.o built as shared instead of static??
add_library(microkernel main.c task.c thread.c paging.c interrupt.c metrics.c containers.c indexing.c salloc.c dynmem.c message.c shared-mem.c bitops.c error.c permissions.c ttrace.c)
set_target_properties(microkernel PROPERTIES COMPILE_FLAGS "-MD")

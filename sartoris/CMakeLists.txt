cmake_minimum_required (VERSION 2.6)

project(Sartoris)

set(AS nasm)

set(SARTORIS_ARCH i386)

if(${SARTORIS_ARCH} STREQUAL "i386")
  set(C_ARCH_FLAG "-m32")
  set(LD_ARCH_FLAG "-melf_i386")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_ARCH_FLAG}")
set(LD_FLAGS "${LD_ARCH_FLAG}")

# #CC = /opt/crosstool/gcc-4.1.0-glibc-2.3.6/i686-unknown-linux-gnu/bin/i686-unknown-linux-gnu-gcc
#LD = /opt/crosstool/gcc-4.1.0-glibc-2.3.6/i686-unknown-linux-gnu/bin/i686-unknown-linux-gnu-ld
#CFLAGS = -m32 -O2 -D FPU_MMX -D _METRICS_ -D PAGING
#LDFLAGS = -melf_i386
#AS = nasm
#ASFLAGS = -f elf -d FPU_MMX -D _METRICS_ -D PAGING

add_definitions(-D FPU_MMX -D _METRICS_ -D PAGING -D __KERNEL__ )


include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/arch/${SARTORIS_ARCH}/include")
add_subdirectory("${PROJECT_SOURCE_DIR}/arch/${SARTORIS_ARCH}")
add_subdirectory("${PROJECT_SOURCE_DIR}/mk")

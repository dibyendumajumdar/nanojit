cmake_minimum_required(VERSION 2.8)
project(lirasm)

enable_language(CXX)
enable_language(C)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  add_definitions(
    -DAVMPLUS_IA32
  )
else(CMAKE_SIZEOF_VOID_P EQUAL 8)
  add_definitions(
    -DAVMPLUS_64BIT=1
    -DAVMPLUS_AMD64=1
  )
endif()

set (NANOJIT_ARCH X64)
add_definitions (
	-DAVMPLUS_WIN32=1
	-DFEATURE_NANOJIT=1
	-DVMCFG_SSE2=1
	)

#if (CMAKE_BUILD_TYPE MATCHES DEBUG)
    add_definitions(
    	-DDEBUG=1 
    	-D_DEBUG=1 
    	-DNJ_VERBOSE=1)
#endif()

#undef DEBUG
#undef AVMPLUS_AMD64
#undef AVMPLUS_64BIT
#undef AVMPLUS_ARM
#undef AVMPLUS_IA32
#undef AVMPLUS_LINUX
#undef AVMPLUS_OS2
#undef AVMPLUS_PPC
#undef AVMPLUS_MIPS
#undef VMCFG_SH4
#undef VMCFG_FLOAT
#undef VMCFG_NEON
#undef AVMPLUS_SPARC
#undef SOLARIS
#undef AVMPLUS_UNIX
#undef WINVER
#undef _CRT_NONSTDC_NO_DEPRECATE
#undef _CRT_NONSTDC_NO_WARNINGS
#undef _CRT_SECURE_NO_DEPRECATE
#undef _CRT_SECURE_NO_WARNINGS
#undef WIN32
#undef _WIN32_IE
#undef _WIN32_WINNT

include_directories(
    .
    utils/nanojit-lirasm/VMPI.nj        
    nanojit
)

set (NATIVE_SRC
		nanojit/Native${NANOJIT_ARCH}.cpp
	)
set (LIB_SRCS
		nanojit/Allocator.cpp
        nanojit/Assembler.cpp
        nanojit/CodeAlloc.cpp
        nanojit/Containers.cpp
        nanojit/Fragmento.cpp
        nanojit/LIR.cpp
        nanojit/njconfig.cpp
        nanojit/RegAlloc.cpp
        utils/nanojit-lirasm/VMPI.nj/VMPI.cpp
        utils/nanojit-lirasm/VMPI.nj/avmplus.cpp
        vprof/vprof.cpp
        AVMPI/float4Support.cpp
    )
set (LIRASM_SRCS utils/nanojit-lirasm/lirasm/lirasm.cpp)

add_library(nanojit ${LIB_SRCS} ${NATIVE_SRC})

add_executable(lirasm ${LIRASM_SRCS})
target_link_libraries(lirasm nanojit)

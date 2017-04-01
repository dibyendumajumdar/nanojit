cmake_minimum_required(VERSION 2.8)
project(nanojitextra)

enable_language(CXX)
enable_language(C)

message(STATUS "OS type is ${CMAKE_SYSTEM_NAME}")
message(STATUS "System processor is ${CMAKE_HOST_SYSTEM_PROCESSOR}")
message(STATUS "Build type is ${CMAKE_BUILD_TYPE}")

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  add_definitions (
    -DAVMPLUS_UNIX=1
    -DAVMPLUS_LINUX=1
  )
  set(EXTRA_LIBS dl)
elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  add_definitions (
    -DAVMPLUS_UNIX=1
  )    
elseif(CMAKE_SYSTEM_NAME STREQUAL Windows)
  add_definitions (
    -DAVMPLUS_WIN32=1
  )
else()
  message(FATAL_ERROR "The build only supports Windows, Mac OSX and Linux currently")
endif()

string(TOUPPER ${CMAKE_HOST_SYSTEM_PROCESSOR} UPPERCASE_CMAKE_HOST_SYSTEM_PROCESSOR)

if(UPPERCASE_CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL X86_64 OR UPPERCASE_CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL AMD64)
  add_definitions(
    -DAVMPLUS_64BIT=1
    -DAVMPLUS_AMD64=1
    -DVMCFG_SSE2=1
  )
  set (NANOJIT_ARCH X64)
elseif(UPPERCASE_CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL I686)
  add_definitions(
    -DAVMPLUS_IA32
    -DVMCFG_SSE2=1
  )
  set (NANOJIT_ARCH i386)  
else()
  message(FATAL_ERROR "The build only supports AMD64 and I386 currently")
endif()

add_definitions (
  -DFEATURE_NANOJIT=1
)

if (CMAKE_BUILD_TYPE)
  string(TOUPPER ${CMAKE_BUILD_TYPE} UPPERCASE_CMAKE_BUILD_TYPE)

  if (UPPERCASE_CMAKE_BUILD_TYPE STREQUAL DEBUG)
    add_definitions(
      -D_DEBUG=1 
      -DDEBUG=1 
      -DAVMPLUS_VERBOSE=1
    )
  endif()
else()
  message(FATAL_ERROR "Unknown build type! Set CMAKE_BUILD_TYPE to DEBUG, CHECKED, RELEASE, or RELWITHDEBINFO!")
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    utils/nanojit-lirasm/VMPI.nj        
    nanojit
    nanojitextra
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
set (EXT_SRCS
        nanojitextra/spi.cpp
        nanojitextra/context.cpp)

set (LIRASM_SRCS utils/nanojit-lirasm/lirasm/lirasm.cpp)

set (NANOJITEXTRA_HEADERS nanojitextra/nanojitextra.h)

add_library(nanojit ${LIB_SRCS} ${NATIVE_SRC})
target_link_libraries(nanojit ${EXTRA_LIBS})

add_library(nanojitextra ${LIB_SRCS} ${NATIVE_SRC} ${EXT_SRCS})
target_link_libraries(nanojitextra ${EXTRA_LIBS})

add_executable(lirasm ${LIRASM_SRCS})
target_link_libraries(lirasm nanojit)

add_executable(example1 samples/example1.cpp)
target_link_libraries(example1 nanojitextra)

install(FILES ${NANOJITEXTRA_HEADERS}
        DESTINATION include/nanojit)
install(TARGETS nanojitextra lirasm
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)
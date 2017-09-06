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

if (MSVC)
    set(LIBRARY_TYPE STATIC)
else()
    set(LIBRARY_TYPE SHARED)
endif()

if (UNIX AND NOT APPLE)
    # -fsanitize=bounds -fsanitize=alignment -fsanitize=object-size
    # set(SANITIZER_FLAGS "-fsanitize=address")
    set(CMAKE_C_FLAGS "${SANITIZER_FLAGS} -fno-omit-frame-pointer -std=c99 -O2 -g -Wall -Wno-comment ${CXX_OPTIONS}")
    set(CMAKE_C_FLAGS_DEBUG "${SANITIZER_FLAGS} -fno-omit-frame-pointer -std=c99 -O0 -g3 -Wall -Wno-comment ${CXX_OPTIONS}")
    set(CMAKE_C_FLAGS_RELEASE "-std=c99 -O3 -Wall -Wno-comment ${CXX_OPTIONS}")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${SANITIZER_FLAGS} -std=c99 -O1 -g3 -Wall  ${CXX_OPTIONS}")
    set(CMAKE_CXX_FLAGS "${SANITIZER_FLAGS} -fno-omit-frame-pointer -O0 -g3 -Wall -Wno-narrowing -Wno-sign-compare -std=c++11 -fno-exceptions ${CXX_OPTIONS}")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -Wno-narrowing -Wno-sign-compare -std=c++11 -fno-exceptions ${CXX_OPTIONS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${SANITIZER_FLAGS} -fno-omit-frame-pointer -O0 -g3 -Wall -Wno-narrowing -Wno-sign-compare -std=c++11 -fno-exceptions ${CXX_OPTIONS}")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${SANITIZER_FLAGS} -O1 -g3 -Wall -Wno-narrowing -Wno-sign-compare -std=c++11 -fno-exceptions ${CXX_OPTIONS}")
endif ()

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
        nanojitextra/nanojitextra.cpp)

set (LIRASM_SRCS utils/nanojit-lirasm/lirasm/lirasm.cpp)

set (NANOJITEXTRA_HEADERS nanojitextra/nanojitextra.h)

add_library(nanojit ${LIBRARY_TYPE} ${LIB_SRCS} ${NATIVE_SRC})
target_link_libraries(nanojit ${EXTRA_LIBS})
if(APPLE)
	set_target_properties(nanojit PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

add_library(nanojitextra ${LIBRARY_TYPE} ${LIB_SRCS} ${NATIVE_SRC} ${EXT_SRCS} ${NANOJITEXTRA_HEADERS})
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

# NanoJIT
NanoJIT is a small, cross-platform C++ library that emits machine code. It is part of [Adobe ActionScript](https://github.com/adobe/avmplus) 
and used to be part of [Mozilla SpiderMonkey](https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey/Internals/Tracing_JIT) but is no longer used in SpiderMonkey.

## High level overview
NanoJIT defines its own linear IR called LIR. This is not an SSA IR as there are no phi nodes. Compared with LLVM IR, the NanoJIT IR is low level. There are only primitive types such as 32-bit and 64-bit integers, doubles and floats, and pointers. Users have to manage complex types on their own.

The NanoJIT IR is also restricted by platform, e.g. some instructions are only available on 64-bit platforms. 

The main unit of compilation in NanoJIT is a Fragment - which can be thought of as a chunk of code. You can make a function out of a fragment by providing a start instruction and appropriate ret instructions. But Fragments need not be functions. I believe this flexibility stems from the fact that NanoJIT was designed to be used in a tracing JIT.

The documentation on NanoJIT is sparse or non-existent, making it hard to get started. The project aims to provide a [simpler, documented C API](https://github.com/dibyendumajumdar/nanojit/blob/master/nanojitextra/nanojitextra.h) to make it easier to use NanoJIT.

## Project news

* Sep-2017: First alpha release of NanoJITExtra C API
* Currently I am concentrating on X86-64 architecture - in particular I have no ability to test the non X86 architectures
* April-2017: Added support for 64-bit integer multiply, divide and modulus operators in X64 LIR. Not available on other architectures.

## Playing with NanoJIT

NanoJIT comes with a nice tool called lirasm. This is a command line tool that allows you to run a script containing NanoJIT IR instructions. For example, say you want a function that adds its two arguments. We can write this as follows:

```
; this is our add function
; it takes two parameters
; and returns the sum of the two
; note that this script will only run on 64-bit platforms as q2i instruction is
; not available on 32-bit platforms

; the .begin and .end instructions tell lirasm to generate the function prologue and epilogue

.begin add
p1 = paramp 0 0		     ; the first '0' says that this is the 0th parameter 
                       ; the second argument '0' says this is a parameter
p2 = paramp 1 0		     ; the second parameter
x  = q2i p1            ; convert from int64 to int32
                       ; this instruction will only work on 64-bit machines
                       ; it ensures that the script will fail to compile on 32-bit arch
y  = q2i p2            ; convert from int64 to int32
sum = addi x y	       ; add
reti sum
.end

; this is our main function
; we just call add with 200, 100
.begin main
oneh = immi 100		     ; constant 100
twoh = immi 200		     ; constant 200
res = calli add fastcall twoh oneh     ; call function add
reti res
.end
```

If you save above script to a file named add.in, then you can run lirasm as follows:

```
lirasm add.in
```

You can see the generated code by running:

```
lirasm -v add.in
```

## Example using NanoJITExtra API
This project is creating a simplified C API for NanoJIT - I call this NanoJITExtra. The API is defined in [nanojitextra.h](https://github.com/dibyendumajumdar/nanojit/blob/master/nanojitextra/nanojitextra.h). *Note* that this is work in progress.

```c++

NJXContextRef jit = NJX_create_context(true);

const char *name = "add";
typedef int (*functype)(NJXParamType, NJXParamType);

// Create a function builder
NJXValueKind args[2] = {NJXValueKind_I, NJXValueKind_I};
NJXFunctionBuilderRef builder = NJX_create_function_builder(jit, name, NJXValueKind_I, args, 2, true);

auto x = NJX_get_parameter(builder, 0); /* arg1 */
auto y = NJX_get_parameter(builder, 1); /* arg2 */
auto result = NJX_addi(builder, x, y);       /* result = x + y */
auto ret = NJX_reti(builder, result);        /* return result */

functype f = (functype)NJX_finalize(builder);

NJX_destroy_function_builder(builder);

assert(f);
assert(f(100, 200) == 300);

NJX_destroy_context(jit);

```

## More examples
There are bunch of [tests](https://github.com/dibyendumajumdar/nanojit/tree/master/utils/nanojit-lirasm/lirasm/tests) that come with the lirasm tool. These are examples of LIR scripts.

The samples folder contains an [example program](https://github.com/dibyendumajumdar/nanojit/blob/master/samples/example1.cpp) that illustrates using the NanoJITExtra C API.

I am using NanoJIT as the backend for a C compiler - you can see more examples of [NanoJIT LIR here](https://github.com/dibyendumajumdar/dmr_c/tree/master/nanojit-backend).

## Building NanoJIT
While the goal of this project is to create a standalone build of NanoJIT, the original folder structure of avmplus is maintained so that merging upstream changes is easier.

The new build is work in progress. A very early version of CMakeLists.txt is available, this has been tested only on Windows 10 with Visual Studio 2017. The aim is to initially support the build on X86_64 processors, and Windows, Linux and Mac OSX.  

To create Visual Studio project files do following:

```
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install -G "Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE=Debug ..
```

On Linux the command sequences is:

```
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug ..
```

Building the project will result in a standalone NanoJIT and NanoJITExtra libraries, and the executable lirasm which can be used to assemble and run standalone code snippets as described above. Assuming you specified the CMAKE_INSTALL_PREFIX you can install the header files and the library using your build script.

## Using NanoJIT

Once you have built the library all you need is to statically link the library, and include the nanojitextra.h header file. Note that the API is till being developed so not all API calls are exposed yet. 

Using NanoJIT on its own is a bit complicated mainly due to the requirement to provide variable liveness information as
described below. Addditionally the resolution of jumps to labels also requires some pre-processing.

It is therefore far easier to use a front-end to generate the NanoJIT LIR - a C front-end is being developed in the 
project [dmr_C](https://github.com/dibyendumajumdar/dmr_c/tree/master/nanojit-backend). If you would still like to use 
NanoJIT directly then please read following carefully.

### Insert Liveness information 
The following information is from Edwin Smith (original Nanojit architect):

The register allocator computes virtual register liveness as it runs, while it
is scanning LIR bottom-up. To prevent the allocator from thinking a
register is available when it is not, following actions are needed:

a) Mark function parameters as live after all code is emitted for the function.

b) If a virtual register is being defined before the loop entry
point, and used inside a loop, then it's live range must cover the whole loop.
the frontend compiler must insert LIR_live at the loop jumps (back edges)
to extend the live range.

Example: Suppose you have a jump to block B. LIR_live for B's live-in
registers, just before the jump, should be added (only needed for backwards
jumps though). Note that if the jump is in B1 and the target is B2, you
need LIR_live for B2's live-in registers.

### Jumps and Labels
The instruction set requires setting labels as jump targets. There is no concept of basic blocks as in LLVM, but a basic block can be simulated by having a sequence of code with a label at the beginning and a jump at the end.

The code generator inserts the next instruction into the _current_ position within the LIR buffer. You may not yet have the target instruction defined yet as most jumps are forward jumps. Hence following procedure must be followed:

* When you need to insert a jump, initially set jump target to NULL. This is okay. But keep track somewhere (e.g. in a memory structure) the logical target (e.g. the label name) for that jump target.  
* Assign labels to the start of each basic block as you generate code for each basic block - these will become jump targets. Maintain a map of labels names to instructions. 
* After code generation is completed go back through the list of jumps you created in step 1, and set the targets to the labels which are now in place. You use the map created in step 2 to locate the label instructions.

### NanoJIT does not handle complex types
The NanoJIT IR works at the level of integers, floats and pointers. Complex structures have to be managed by the front-end
by generating appropriate load/store sequences.

### JIT Functions can only take integer/pointer arguments
At least on X64 a limitation in NanoJIT is that JIT compiled functions can only take a limited number of arguments. On Win64 the limit is upto 4 integer or pointer arguments. On UNIX platforms it is 6 arguments, again only integer or pointer values. These
limitations arise as the current implementation only looks at the first 4/6 registers for arguments as per the X64 ABI.

A JIT function should be able to return a double or float though - but this is something that is yet to be verified.

### External C functions can only take upto 8 arguments
External C functions called from JIT code can only take upto 8 arguments, although in this case it it possible to
to pass double or float values. Return values can also be double or float.

## Documentation
A secondary goal of this project is to create some documentation of the standalone library, and document how it can be used. 

* [Main Components in NanoJIT](https://github.com/dibyendumajumdar/nanojit/blob/master/docs/overview.md)
* [LIR Op Codes](https://github.com/dibyendumajumdar/nanojit/blob/master/docs/nanjit-opcodes.md)

## Why NanoJIT?
It seems that NanoJIT is one of the rare examples of a small cross-platform standalone JIT library that can be used outside of the original project. It also matters that the license is not GPL. Finally it has been in production use in ActionScript and Adobe Flash for some time so one hopes that most bugs have been ironed out.

## Why not NanoJIT?
Support is virtually non-existent. The original architect/developer Edwin Smith is no longer at Adobe, and works at [Facebook on HHVM](https://www.youtube.com/watch?v=GT4LxjJd2Ac). Although he is not involved with NanoJIT anymore, Edwin has graciously answered some of my questions. The Adobe team do not seem to respond to [issues](https://github.com/adobe/avmplus/issues).


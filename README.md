# nanojit
Nanojit is a small, cross-platform C++ library that emits machine code. It is part of [Adobe ActionScript](https://github.com/adobe/avmplus) 
and used to be part of [Mozilla SpiderMonkey](https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey/Internals/Tracing_JIT) but is no longer used in SpiderMonkey.

## High level overview
Nanojit defines its own linear IR called LIR. This is not an SSA IR as there are no phi nodes. Compared with LLVM IR, the Nanojit IR is low level. There are only primitive types such as 32-bit and 64-bit integers, doubles and floats, and pointers. Users have to manage complex types on their own.

The Nanojit IR is also restricted by platform, e.g. some instructions are only available on 64-bit platforms. 

The main unit of compilation in Nanojit is a Fragment - which can be thought of as a chunk of code. You can make a function out of a fragment by providing a start instruction and appropriate ret instructions. But Fragments need not be functions. I believe this flexibility stems from the fact that Nanojit was designed to be used in a tracing JIT.

A limitation (or complexity) in Nanojit is that function parameters are always the architecture word size, i.e. 64-bit on 64-bit platforms, and 32-bit on 32-bit platforms. This means that you cannot directly pass a double as a parameter on a 32-bit platform! However the workaround is simple, just pass a pointer to a struct that contains the arguments. Of course Nanojit does not understand structs so you need to call the relevant memory load/store operations.

The documentation on Nanojit is sparse or non-existent, making it hard to get started. I hope to provide a simpler, documented api to make it easier to use Nanojit.

## Playing with Nanojit

Nanojit comes with a nice tool called lirasm. This is a command line tool that allows you to run a script containing Nanojit IR instructions. For example, say you want a function that adds its two arguments. We can write this as follows:

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

## Example using NanoJitExtra api
This project is creating a simplified API for nanojit - we call this nanjitexra. The API is defined in [nanojitextra.h](https://github.com/dibyendumajumdar/nanojit/blob/master/nanojitextra/nanojitextra.h). *Note* that this is work in progress.

```c++

NanoJitContext jit(true);

typedef int(*functype)(parameter_type, parameter_type);

// Create a function builder
FunctionBuilder builder(jit, "add", true /*optimize*/);

auto param1 = builder.insertParameter();   /* arg1 */
auto param2 = builder.insertParameter();   /* arg2 */
auto x = builder.q2i(param1);              /* x = (int) arg1 */
auto y = builder.q2i(param2);              /* y = (int) arg2 */
auto result = builder.addi(x, y);          /* result = x = y */
auto ret = builder.reti(result);           /* return result */

functype f = (functype)builder.finalize();

assert(f);
assert(f(100, 200) == 300);
```

## Building Nanojit
While the goal of this project is to create a standalone build of Nanojit, the original folder structure of avmplus is maintained so that merging upstream changes is easier.

The new build is work in progress. A very early version of CMakeLists.txt is available, this has been tested only on Windows 10 with Visual Studio 2017. The aim is to initially support the build on X86_64 processors, and Windows, Linux and Mac OSX.  

To create Visual Studio project files do following:

```
mkdir build
cd build
cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE=Debug ..
```

Building the project will result in a standalone nanojit and naojitextra libraries, and the executable lirasm which can be used to assemble and run standalone code snippets as described above. 

## Documentation
A secondary goal of this project is to create some documentation of the standalone library, and document how it can be used. 

* [Main Components in NanoJIT](https://github.com/dibyendumajumdar/nanojit/blob/master/docs/overview.md)
* [LIR op codes](https://github.com/dibyendumajumdar/nanojit/blob/master/docs/nanjit-opcodes.md)

## Why nanojit?
It seems that this is perhaps the only small JIT library that is available. 

# nanojit Overview

## Acknowledgment

This document is based upon the [Mozilla article](https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey/Internals/Tracing_JIT). 

## LIR
The nanojit/LIR.cpp and nanojit/LIR.h files define the intermediate representation LIR, which is used as input to nanojit. LIR is a conventional three-address, linear code. It is similar to SSA but not exactly so as it lacks phi nodes. A single instruction of LIR is called a LIns, short for "LIR instruction". The LIns values are inserted into a LIR buffer, itself contained within a logical fragment, and the nanojit compilation pipeline and Assembler transforms the LIns values into NIns values (i.e. machine code).

## Fragment
A Fragment represents a chunk of code. A fragment can be made a function by giving it a LIR_start and appropriate return instructions, but it need not always be a function, i.e., it can be a fragment of a function.  

## Assembler
The nanojit/Assembler.cpp and nanojit/Assembler.h files define the class Assembler, which transforms LIns values into NIns values. In other words, an Assembler transforms LIR code into native code. An Assembler is also able to modify existing fragments of native code, by rewriting native jump instructions to jump to new locations. In this way the Assembler can "patch together" multiple fragments, so that program control can flow from one fragment into another, or back out of generated code and into the interpreter.
An Assembler reads or writes native code. Therefore an Assembler contains several machine-specific methods which are implemented in the accompanying nanojit/Native*.* files.
An Assembler runs in a single pass over its input, transforming one LIns value to zero or more NIns values. It is important to keep in mind that this pass runs backwards from the last LIns in the input LIR code to the first, generating native code in reverse. Running backwards sometimes makes the logic difficult to follow, but it is an essential factor in maintaining the Assembler's high speed and small size.

## RegAlloc
Nanojit's register allocator. This is a local register allocator, meaning that it does not allocate registers across basic blocks. It is correspondingly very simple: register allocation is done immediately, step-by-step as code is being generated. A running tally is kept of assignments between registers and LIR operands, and any time a new LIR operand is required in a register a new one is assigned from the list of free registers. When all registers are in use, the least-often-used register currently in use is spilled.

## Native
The files Nativei386.h, Nativei386.cpp, NativeARM.h, NativeARM.cpp, etc. each define architecture-specific methods within the Assembler class. Only one architecture-specific variant is included into any given build of the Assembler; the architecture is selected and fixed when the build is configured.
The architecture-specific methods found in these files are the only functions within nanojit or TraceMonkey that emit raw bytes of machine-code into memory.

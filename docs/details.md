# Some random titbits

## Function parameters

### JIT Functions can only take integer/pointer arguments
At least on X64 a limitaton in NanoJIT is that JIT functions can only take a limited number of parameters. On Win64 this is
4 integer or pointer arguments. On UNIX platforms it is 6 parameters, again limited to integer or pointer values. These
limitations arise as the current implementation only looks at the first 4/6 registers for parameters as per the X64 ABI.

A JIT function should be able to return a double or float though - but this is something that is to be verified.

### External C functions can only take upto 8 arguments
External C functions called from JIT code can only take upto 8 arguments, although in this case it it possible to
to pass double or float values. Return values can also be double or float.

### All parameters are fixed size
The LIR writer does not allow specifying whether a parameter is 32-bit or 64-bit. Apparently the size is set automatically based 
on machine architecture. See mozilla [bug 541232](https://bugzilla.mozilla.org/show_bug.cgi?id=541232). 

This limitation means that for a JITed function, parameters are always the architecture word size, i.e. 64-bit on 64-bit platforms, and 32-bit on 32-bit platforms. As described above, a JITed function can only accept integer or pointer arguments therefre if you need to pass double values, just pass a pointer to a struct that contains the arguments. Of course Nanojit does not understand structs so you need to call the relevant memory load/store operations.

### Liveness requirements 
I found it necessary to copy function parameters to the stack, as the parameter value appears to not be preserved across jumps. 

Update: I understand from Edwin Smith (original Nanojit architect) that the reason why a parameter may be getting clobberred is because:

> The register allocator computes virtual register liveness as it runs, while it
is scanning LIR bottom-up. If the parameter register is being reused, it's
because the register allocator thinks it's available.  Since it's running bottom
up, it will see the uses of a register before the definition, if the register is
being used at all.

> Loops are tricky. If a virtual register is being defined before the loop entry
point, and used inside a loop, then it's live range must cover the whole loop.
the frontend compiler must insert LIR_live at the loop jumps (back edges)
to extend the live range. see LIR_livei, livep, etc..

## Jumps and Labels
The instruction set requires setting labels as jump targets. There is no concept of basic blocks as in LLVM, but a basic block can be simulated by having a sequence of code with a label at the beginning and a jump at the end.

As far as I can see the code generation always inserts the next instruction into the current position within the LIR buffer, so that the way labels and jumps need to be handled is like this:

* When you need to insert a jump, initially set jump target to NULL. This is okay. But keep track in a memory structure the logical target (e.g. the label name) for that jump target.  
* Assign labels to the start of each basic block as you generate code for each basic block - these will become jump targets. Maintain a map of labels names to instructions.
* After code generation is completed go back through the list of jumps you created in step 1, and set the targets to the labels which are now in place. You use the map created in step 2 to locate the label instructions.


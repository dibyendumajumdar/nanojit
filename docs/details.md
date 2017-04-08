# Some random titbits

## Function parameters

The LIR writer does not allow specifying whether a parameter is 32-bit or 64-bit. Apparently the size is set automatically based 
on machine architecture. See mozilla [bug 541232](https://bugzilla.mozilla.org/show_bug.cgi?id=541232). 

I have also found it necessary to copy function parameters to the stack, as the parameter value appears to not be preserved across jumps. This could be my misunderstanding though.

## Jumps and Labels

The instruction set requires setting labels as jump targets. There is no concept of basic blocks as in LLVM, but a basic block can be simulated by having a sequence of code with a label at the beginning and a jump at the end.

As far as I can see the code generation always inserts the next instruction into the current position within the LIR buffer, so that the way labels and jumps need to be handled is like this:

* When you need to insert a jump, initially set jump target to NULL. This is okay. But keep track in a memory structure the logical target (e.g. the label name) for that jump target.  
* Assign labels to the start of each basic block as you generate code for each basic block - these will become jump targets. Maintain a map of labels names to instructions.
* After code generation is completed go back through the list of jumps you created in step 1, and set the targets to the labels which are now in place. You use the map created in step 2 to locate the label instructions.



# nanojit OpCodes

In Nanojit, LIR is the source language for compilation to machine code. LIR stands for low-level intermediate representation.  It is a typed assembly language.
The LIR instruction set is best learnt by reading nanojit/LIRopcode.tbl.  Code for manipulating LIR is in nanojit/LIR.h.

# LIR OpCodes

Opcodes use type-indicators suffixes that are loosely based on C type names:

* - 'c': "char",  ie. 8-bit integer
* - 's': "short", ie. 16-bit integer
* - 'i': "int",   ie. 32-bit integer
* - 'q': "quad",  ie. 64-bit integer
* - 'u': "unsigned", is used as a prefix on integer type-indicators when necessary
* - 'f': "float",   ie. 32-bit floating point value
* -'f4': "float4",  ie. 128-bit SIMD value containing 4 single-precision floating point values
* - 'd': "double",  ie. 64-bit floating point value
* - 'p': "pointer", ie. an int on 32-bit machines, a quad on 64-bit machines
 
'p' opcodes are all aliases of int and quad opcodes, they're given in LIR.h
and chosen according to the platform pointer size.

Certain opcodes aren't supported on all platforms

## Miscellaneous operations

| Opcode | Todo | Return Type | Featured | Description |
| --- | --- | --- | --- | --- |
| start | Op0 | V | | start of a fragment |
| regfence |  Op0 | V | | A register fence causes no code to be generated, but it affects register allocation so that no registers are live when it is reached. |
| unreachable | Op0 | V | |	indicate that this location cannot be reached (no live regs here) |
| skip | Sk | V | | links code chunks |
| parami | P | I | 32-bit | load an int parameter (register or stack location) |
| paramq | P | Q | 64-bit | load a quad parameter (register or stack location) |
| allocp | IorF | P | | allocate stack space (result is an address) |
| reti | Op1 | V | | return an int |
| retq | Op1 | V | 64-bit | return a quad |
| retd | Op1 | V | | return a double |
| retf | Op1 | V | | return a float |
| retf4 | Op1 | V | | return a float4 |
| livei | Op1 | V | | extend live range of an int |
| liveq | Op1 | V | 64-bit | extend live range of a quad |
| lived | Op1 | V | | extend live range of a double |
| livef | Op1 | V | | extend live range of a float |
| livef4 | Op1 | V | | extend live range of a float4 |
| file | Op1 | V | | [VTune] source filename for debug symbols |
| line | Op1 | V | | [VTune] source line number for debug symbols |
| pc | Op1 | V | | [Shark] record the machine address of this instruction |
| comment | Op1 | V | | a comment shown, on its own line, in LIR dumps |
| safe | Safe | V | | deoptimization safepoint |
| endsafe | Safe | V | | deoptimization safepoint |

## Loads and stores

| Opcode | Todo | Return Type | Featured | Description |
| --- | --- | --- | --- | --- |
| ldc2i | Ld | I | | load char and sign-extend to an int |
| lds2i | Ld | I | | load short and sign-extend to an int |
| lduc2ui | Ld | I | | load unsigned char and zero-extend to an unsigned int |
| ldus2ui | Ld | I | | load unsigned short and zero-extend to an unsigned int |
| ldi | Ld | I | | load int |
| ldq | Ld | Q | 64-bit | load quad |
| ldd | Ld | D | | load double |
| ldf | Ld | F | | load float |
| ldf2d | Ld | D | | load float and extend to a double |
| ldf4 | Ld | F4 | | load float4 (SIMD, 4 floats) |
| sti2c | St | V | | store int truncated to char |
| sti2s | St | V | | store int truncated to short |
| sti | St | V | | store int |
| stq | St | V | 64-bit | store quad |
| std | St | V | |  store double |
| std2f|     St|    V|     | |  store double as a float (losing precision) |
| stf|       St|    V|     | |  store float |
| stf4|      St|    V|     | |  store float4 (SIMD|  4 floats) |


## Calls
| Opcode | Todo | Return Type | Featured | Description |
| --- | --- | --- | --- | --- |
| callv|     C|     V|     |  call subroutine that returns void |
| calli|     C|     I|     |  call subroutine that returns an int |
|callq|     C|     Q| 64-bit   |  call subroutine that returns a quad |
| calld|     C|     D|    |  call subroutine that returns a double |
| callf|     C|     F|    |  call subroutine that returns a float |
| callf4|    C|     F4|   |  call subroutine that returns a float4 |

## Branches and labels

'jt' and 'jf' must be adjacent so that (op ^ 1) gives the opposite one.
Static assertions in LIR.h check this requirement.

| Opcode | Todo | Return Type | Featured | Description |
| --- | --- | --- | --- | --- |
| j|         Op2|   V|     |  jump always |
| jt|        Op2|   V|     |  jump if true |
| jf|        Op2|   V|     |  jump if false |
| jtbl|      Jtbl|  V|     |  jump to address in table |
| label|     Op0|   V|     |  a jump target (no machine code is emitted for this) |

## Guards
'xt' and 'xf' must be adjacent so that (op ^ 1) gives the opposite one.
Static assertions in LIR.h check this requirement.

OP_UN (align_guards)

| Opcode | Todo | Return Type | Featured | Description |
| --- | --- | --- | --- | --- |
| x|         Op2|   V|     |  exit always |
| xt|        Op2|   V|     | exit if true |
| xf|        Op2|   V|     | exit if false |
| xbarrier|  Op2|   V|     | A LIR_xbarrier cause no code to be generated, but it acts like a never-taken guard in that it inhibits certain optimisations, such as dead stack store elimination. |

## Immediates

| Opcode | Todo | Return Type | Featured | Description |
| --- | --- | --- | --- | --- |
| immi|      IorF|  I|     | int immediate |
|immq|      QorD|  Q| 64-bit    | quad immediate |
| immd|      QorD|  D|     | double immediate |
| immf|      IorF|  F|     | float immediate |
| immf4|     F4|    F4|    | float4 immediate |

## Comparisons

All comparisons return an int:  0 on failure and 1 on success.
Within each type group, order must be preserved so that, except for eq*, (op
^ 1) gives the opposite one (eg. lt ^ 1 == gt).  eq* must have odd numbers
for this to work.  They must also remain contiguous so that opcode range
checking works correctly.  Static assertions in LIR.h check these
requirements.

OP_UN (align_eqi)

| Opcode | Todo | Return Type | Featured | Description |
| --- | --- | --- | --- | --- |
| eqi|       Op2|   I|     |          int equality | 
| lti|       Op2|   I|     |   signed int less-than |
| gti|       Op2|   I|     |   signed int greater-than |
| lei|       Op2|   I|     |   signed int less-than-or-equal |
| gei|       Op2|   I|     |   signed int greater-than-or-equal |
| ltui|      Op2|   I|     | unsigned int less-than |
| gtui|      Op2|   I|     | unsigned int greater-than |
| leui|      Op2|   I|     | unsigned int less-than-or-equal |
| geui|      Op2|   I|     | unsigned int greater-than-or-equal |

OP_UN_64(align_eqq)

| Opcode | Todo | Return Type | Featured | Description |
| --- | --- | --- | --- | --- |
|eqq|       Op2|   I| 64-bit    |          quad equality |
|ltq|       Op2|   I| 64-bit    |   signed quad less-than |
|gtq|       Op2|   I| 64-bit    |   signed quad greater-than |
|leq|       Op2|   I| 64-bit    |   signed quad less-than-or-equal |
|geq|       Op2|   I| 64-bit    |   signed quad greater-than-or-equal |
|ltuq|      Op2|   I| 64-bit    | unsigned quad less-than |
|gtuq|      Op2|   I| 64-bit    | unsigned quad greater-than |
|leuq|      Op2|   I| 64-bit    | unsigned quad less-than-or-equal |
|geuq|      Op2|   I| 64-bit    | unsigned quad greater-than-or-equal |

OP_UN_64(align_eqd)

| Opcode | Todo | Return Type | Featured | Description |
| --- | --- | --- | --- | --- |
| eqd|       Op2|   I|     | double equality |
| ltd|       Op2|   I|     | double less-than |
| gtd|       Op2|   I|     | double greater-than |
| led|       Op2|   I|     | double less-than-or-equal |
| ged|       Op2|   I|     | double greater-than-or-equal |

OP_UN (align_eqf)

| Opcode | Todo | Return Type | Featured | Description |
| --- | --- | --- | --- | --- |
| eqf|       Op2|   I|     | float equality |
| ltf|       Op2|   I|     | float less-than |
| gtf|       Op2|   I|     | float greater-than |
| lef|       Op2|   I|     | float less-than-or-equal |
| gef|       Op2|   I|     | float greater-than-or-equal |
| eqf4|      Op2|   I|     | float4 equality |

Note: we don't do lt/gt/le/ge comparisons on float4 values

## Arithmetic
| Opcode | Todo | Return Type | Featured | Description |
| --- | --- | --- | --- | --- |
| negi|      Op1|   I|     | negate int |
| addi|      Op2|   I|     | add int |
| subi|      Op2|   I|     | subtract int |
| muli|      Op2|   I|     | multiply int |
| divi|      Op2|   I|  32-bit X86   | divide int |
| modi|      Op1|   I| 32-bit X86    | modulo int. LIR_modi is a hack.  It's only used on i386/X64.  The operand is the result of a LIR_divi because on i386/X64 div and mod results are computed by the same instruction. |
| noti|      Op1|   I|     | bitwise-NOT int |
| andi|      Op2|   I|     | bitwise-AND int |
| ori|       Op2|   I|     | bitwise-OR int |
| xori|      Op2|   I|     | bitwise-XOR int |
| lshi|      Op2|   I|     | left shift int. For all three integer shift operations, only the bottom five bits of the second operand are used, and they are treated as unsigned.  This matches x86 semantics. |
| rshi|      Op2|   I|     | right shift int (>>) |
| rshui|     Op2|   I|     | right shift unsigned int (>>>) |
|addq|      Op2|   Q| 64-bit    | add quad |
|subq|      Op2|   Q| 64-bit    | subtract quad |
|mulq|      Op2|   Q| 64-bit X86  | multiply quad |
| divq|      Op2|   Q| 64-bit X86 | divide quad |
| modq|      Op1|   Q| 64-bit X86 | modulo quad. LIR_modq is a hack.  It's only used on i386/X64.  The operand is the result of a LIR_divq because on i386/X64 div and mod results are computed by the same instruction. |
|andq|      Op2|   Q| 64-bit    | bitwise-AND quad |
|orq|       Op2|   Q| 64-bit    | bitwise-OR quad |
|xorq|      Op2|   Q| 64-bit    | bitwise-XOR quad |
|lshq|      Op2|   Q| 64-bit    | left shift quad;           2nd operand is an int. For all three quad shift operations, only the bottom six bits of the second operand are used, and they are treated as unsigned.  This matches x86-64 semantics. |
|rshq|      Op2|   Q| 64-bit    | right shift quad;          2nd operand is an int |
|rshuq|     Op2|   Q| 64-bit    | right shift unsigned quad; 2nd operand is an int |
| negd|      Op1|   D|     | negate double |
| absd|      Op1|   D|     | absolute value of double |
| sqrtd|     Op1|   D|     | sqrt double |
| addd|      Op2|   D|     | add double |
| subd|      Op2|   D|     | subtract double |
| muld|      Op2|   D|     | multiply double |
| divd|      Op2|   D|     | divide double |
| modd|      Op2|   D|     | modulo double. LIR_modd is just a place-holder opcode, ie. the back-ends cannot generate code for it.  It's used in TraceMonkey briefly but is always demoted to a LIR_modl or converted to a function call before Nanojit has to do anything serious with it. |
| negf|      Op1|   F|     | negate float |
| absf|      Op1|   F|     | absolute value of float |
| sqrtf|     Op1|   F|     | sqrt float |
| addf|      Op2|   F|     | add float |
| subf|      Op2|   F|     | subtract float |
| mulf|      Op2|   F|     | multiply float |
| divf|      Op2|   F|     | divide float |
| negf4|     Op1|  F4|     | negate float4 |
| absf4|     Op1|  F4|     | absolute value of float4 |
| sqrtf4|    Op1|  F4|     | sqrt float4 |
| addf4|     Op2|  F4|     | add float4 |
| subf4|     Op2|  F4|     | subtract float4 |
| mulf4|     Op2|  F4|     | multiply float4 |
| divf4|     Op2|  F4|     | divide float4 |
| recipf|    Op1|   F|     | float reciprocal |
| rsqrtf|    Op1|   F|     | float reciprocal square root |
| minf|      Op2|   F|     | float min |
| maxf|      Op2|   F|     | float max |
| cmpgtf4|   Op2|  F4|     | float4.isGreater |
| cmpltf4|   Op2|  F4|     | float4.isLess |
| cmpgef4|   Op2|  F4|     | float4.isGreaterOrEqual |
| cmplef4|   Op2|  F4|     | float4.isLessOrEqual |
| cmpeqf4|   Op2|  F4|     | float4.isEqual |
| cmpnef4|   Op2|  F4|     | float4.isNotEqual |
| recipf4|   Op1|  F4|     | float4 reciprocal |
| rsqrtf4|   Op1|  F4|     | float4 reciprocal square root |
| minf4|     Op2|  F4|     | float4 min |
| maxf4|     Op2|  F4|     | float4 max |
| dotf4|     Op2|   F|     | 4-component dot product |
| dotf3|     Op2|   F|     | 3-component dot product |
| dotf2|     Op2|   F|     | 2-component dot product |
| cmovi|     Op3|   I|     | conditional move int |
|cmovq|     Op3|   Q| 64-bit    | conditional move quad |
| cmovd|     Op3|   D|     | conditional move double |
| cmovf|     Op3|   F|     | conditional move float |
| cmovf4|    Op3|  F4|     | conditional move float4 |

## Conversions

rounding behavior of LIR_d2f is platform-specific

| Platform |    Asm code |       Behavior |
| --- |     --- |        --- |
| x86 w/ x87 |  FST32  |         uses current FP control word (default is rounding) |
| x86 w/ SSE |  cvtsd2ss   |     according to MXCSR register (default is round to nearest) |
| x64 (SSE)  |  cvtsd2ss   |     according to MXCSR register (default is round to nearest) |
| others     | |  not implemented yet |

The rounding behavior of LIR_d2i is platform specific.

| Platform  |   Asm code  |      Behavior |
| --- |    --- |        --- |
| x86 w/ x87 |  fist |           uses current FP control word (default is rounding) |
| x86 w/ SSE |  cvttsd2si   |    performs round to zero (truncate) |
| x64 (SSE) |   cvttsd2si   |       performs round to zero (truncate) | 
| PowerPC | |                      unsupported |
| ARM  |        ftosid    |      round to nearest |
| MIPS |        trunc.w.d  |      performs round to zero (truncate) |
| SH4   |       frtc     |       performs round to zero (truncate) |
| SPARC   |     fdtoi    |       performs round to zero (truncate) |

* round to zero examples:  1.9 -> 1, 1.1 -> 1, -1.1 -> -1, -1.9 -> -1
* round to nearest examples: 1.9 -> 2, 1.1 -> 1, -1.1 -> -1, -1.9 -> -2

| Opcode | Todo | Return Type | Featured | Description |
| --- | --- | --- | --- | --- |
|i2q|       Op1|   Q| 64-bit    | sign-extend int to quad |
|ui2uq|     Op1|   Q| 64-bit    | zero-extend unsigned int to unsigned quad |
|q2i|       Op1|   I| 64-bit    | truncate quad to int (removes the high 32 bits) |
|q2d|       Op1|   D| 64-bit    | convert quad to double |
| i2d|       Op1|   D|     | convert int to double |
| i2f|       Op1|   F|     | convert int to float |
| ui2d|      Op1|   D|     | convert unsigned int to double |
| ui2f|      Op1|   F|     | convert unsigned int to float |
| f2d|       Op1|   D|     | convert float to double |
| d2f|       Op1|   F|     | convert double to float (no exceptions raised) |
| d2i|       Op1|   I|     | convert double to int (no exceptions raised) |
| f2i|       Op1|   I|     | convert float to int (no exceptions raised) |
| f2f4|      Op1|  F4|     | convert float to float4 (no exceptions raised) - essentially copies the float across all elements |
| ffff2f4|   Op4|  F4|     | convert float to float4 (no exceptions raised) - essentially copies the float across all elements |
| f4x|       Op1|   F|     | extract first float from a float4  |
| f4y|       Op1|   F|     | extract second float from a float4  |
| f4z|       Op1|   F|     | extract third float from a float4  |
| f4w|       Op1|   F|     | extract fourth float from a float4  |
| swzf4|     Op1b| F4|     | swizzle float4 according to 8-bit selector |
|dasq|      Op1|   Q| 64-bit    | interpret the bits of a double as a quad |
|qasd|      Op1|   D| 64-bit    | interpret the bits of a quad as a double |

##  Overflow arithmetic

These all exit if overflow occurred.  The result is valid on either path.

| Opcode | Todo | Return Type | Featured | Description |
| --- | --- | --- | --- | --- |
| addxovi|   Op3|   I|     | add int and exit on overflow |
| subxovi|   Op3|   I|     | subtract int and exit on overflow |
| mulxovi|   Op3|   I|     | multiply int and exit on overflow |

These all branch if overflow occurred.  The result is valid on either path.

| Opcode | Todo | Return Type | Featured | Description |
| --- | --- | --- | --- | --- |
| addjovi|   Op3|   I|     | add int and branch on overflow |
| subjovi|   Op3|   I|     | subtract int and branch on overflow |
| muljovi|   Op3|   I|     | multiply int and branch on overflow |
|addjovq|   Op3|   Q| 64-bit    | add quad and branch on overflow |
|subjovq|   Op3|   Q| 64-bit    | subtract quad and branch on overflow |

## SoftFloat

| Opcode | Todo | Return Type | Featured | Description |
| --- | --- | --- | --- | --- |
| dlo2i|     Op1|   I|  SF   | get the low  32 bits of a double as an int |
| dhi2i|     Op1|   I|  SF   | get the high 32 bits of a double as an int |
| ii2d|      Op2|   D|  SF   | join two ints (1st arg is low bits, 2nd is high) |
| hcalli|    Op1|   I|  SF  | LIR_hcalli is a hack that's only used on 32-bit platforms that use SoftFloat.  Its operand is always a LIR_calli, but one that specifies a function that returns a double.  It indicates that the double result is returned via two 32-bit integer registers.  The result is always used as the second operand of a LIR_ii2d. |


## Safepoint Polling

| Opcode | Todo | Return Type | Featured | Description |
| --- | --- | --- | --- | --- |
| memfence|  Op0|  V|  | |
| brsavpc|  Op2|  V|  | branch and save pc |
| restorepc|  Op0|  V|  | |
| pushstate|  Op0|  V|  | |
| popstate|  Op0|  V|  | |

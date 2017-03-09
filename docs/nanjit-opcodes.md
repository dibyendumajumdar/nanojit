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

//---------------------------------------------------------------------------
// Loads and stores
//---------------------------------------------------------------------------
OP___(ldc2i,    Ld,   I,   -1)  // load char and sign-extend to an int
OP___(lds2i,    Ld,   I,   -1)  // load short and sign-extend to an int
OP___(lduc2ui,  Ld,   I,   -1)  // load unsigned char and zero-extend to an unsigned int
OP___(ldus2ui,  Ld,   I,   -1)  // load unsigned short and zero-extend to an unsigned int
OP___(ldi,      Ld,   I,   -1)  // load int
OP_64(ldq,      Ld,   Q,   -1)  // load quad
OP___(ldd,      Ld,   D,   -1)  // load double
OP___(ldf,      Ld,   F,   -1)  // load float
OP___(ldf2d,    Ld,   D,   -1)  // load float and extend to a double
OP___(ldf4,     Ld,   F4,  -1)  // load float4 (SIMD, 4 floats)

OP___(sti2c,    St,   V,    0)  // store int truncated to char
OP___(sti2s,    St,   V,    0)  // store int truncated to short
OP___(sti,      St,   V,    0)  // store int
OP_64(stq,      St,   V,    0)  // store quad
OP___(std,      St,   V,    0)  // store double
OP___(std2f,    St,   V,    0)  // store double as a float (losing precision)
OP___(stf,      St,   V,    0)  // store float
OP___(stf4,     St,   V,    0)  // store float4 (SIMD, 4 floats)


//---------------------------------------------------------------------------
// Calls
//---------------------------------------------------------------------------
OP___(callv,    C,    V,   -1)  // call subroutine that returns void
OP___(calli,    C,    I,   -1)  // call subroutine that returns an int
OP_64(callq,    C,    Q,   -1)  // call subroutine that returns a quad
OP___(calld,    C,    D,   -1)  // call subroutine that returns a double
OP___(callf,    C,    F,   -1)  // call subroutine that returns a float
OP___(callf4,   C,    F4,  -1)  // call subroutine that returns a float4

//---------------------------------------------------------------------------
// Branches and labels
//---------------------------------------------------------------------------
// 'jt' and 'jf' must be adjacent so that (op ^ 1) gives the opposite one.
// Static assertions in LIR.h check this requirement.

OP___(j,        Op2,  V,    0)  // jump always
OP___(jt,       Op2,  V,    0)  // jump if true
OP___(jf,       Op2,  V,    0)  // jump if false
OP___(jtbl,     Jtbl, V,    0)  // jump to address in table

OP___(label,    Op0,  V,    0)  // a jump target (no machine code is emitted for this)

//---------------------------------------------------------------------------
// Guards
//---------------------------------------------------------------------------
// 'xt' and 'xf' must be adjacent so that (op ^ 1) gives the opposite one.
// Static assertions in LIR.h check this requirement.
OP_UN (align_guards)
OP___(x,        Op2,  V,    0)  // exit always
OP___(xt,       Op2,  V,    1)  // exit if true
OP___(xf,       Op2,  V,    1)  // exit if false
// A LIR_xbarrier cause no code to be generated, but it acts like a never-taken
// guard in that it inhibits certain optimisations, such as dead stack store
// elimination.
OP___(xbarrier, Op2,  V,    0)

//---------------------------------------------------------------------------
// Immediates
//---------------------------------------------------------------------------
OP___(immi,     IorF, I,    1)  // int immediate
OP_64(immq,     QorD, Q,    1)  // quad immediate
OP___(immd,     QorD, D,    1)  // double immediate
OP___(immf,     IorF, F,    1)  // float immediate
OP___(immf4,    F4,   F4,   1)  // float4 immediate

//---------------------------------------------------------------------------
// Comparisons
//---------------------------------------------------------------------------

// All comparisons return an int:  0 on failure and 1 on success.
//
// Within each type group, order must be preserved so that, except for eq*, (op
// ^ 1) gives the opposite one (eg. lt ^ 1 == gt).  eq* must have odd numbers
// for this to work.  They must also remain contiguous so that opcode range
// checking works correctly.  Static assertions in LIR.h check these
// requirements.
OP_UN (align_eqi)
OP___(eqi,      Op2,  I,    1)  //          int equality
OP___(lti,      Op2,  I,    1)  //   signed int less-than
OP___(gti,      Op2,  I,    1)  //   signed int greater-than
OP___(lei,      Op2,  I,    1)  //   signed int less-than-or-equal
OP___(gei,      Op2,  I,    1)  //   signed int greater-than-or-equal
OP___(ltui,     Op2,  I,    1)  // unsigned int less-than
OP___(gtui,     Op2,  I,    1)  // unsigned int greater-than
OP___(leui,     Op2,  I,    1)  // unsigned int less-than-or-equal
OP___(geui,     Op2,  I,    1)  // unsigned int greater-than-or-equal

OP_UN_64(align_eqq)
OP_64(eqq,      Op2,  I,    1)  //          quad equality
OP_64(ltq,      Op2,  I,    1)  //   signed quad less-than
OP_64(gtq,      Op2,  I,    1)  //   signed quad greater-than
OP_64(leq,      Op2,  I,    1)  //   signed quad less-than-or-equal
OP_64(geq,      Op2,  I,    1)  //   signed quad greater-than-or-equal
OP_64(ltuq,     Op2,  I,    1)  // unsigned quad less-than
OP_64(gtuq,     Op2,  I,    1)  // unsigned quad greater-than
OP_64(leuq,     Op2,  I,    1)  // unsigned quad less-than-or-equal
OP_64(geuq,     Op2,  I,    1)  // unsigned quad greater-than-or-equal

OP_UN_64(align_eqd)
OP___(eqd,      Op2,  I,    1)  // double equality
OP___(ltd,      Op2,  I,    1)  // double less-than
OP___(gtd,      Op2,  I,    1)  // double greater-than
OP___(led,      Op2,  I,    1)  // double less-than-or-equal
OP___(ged,      Op2,  I,    1)  // double greater-than-or-equal

OP_UN (align_eqf)
OP___(eqf,      Op2,  I,    1)  // float equality
OP___(ltf,      Op2,  I,    1)  // float less-than
OP___(gtf,      Op2,  I,    1)  // float greater-than
OP___(lef,      Op2,  I,    1)  // float less-than-or-equal
OP___(gef,      Op2,  I,    1)  // float greater-than-or-equal

OP___(eqf4,     Op2,  I,    1)  // float4 equality
// Note: we don't do lt/gt/le/ge comparisons on float4 values

//---------------------------------------------------------------------------
// Arithmetic
//---------------------------------------------------------------------------
OP___(negi,     Op1,  I,    1)  // negate int
OP___(addi,     Op2,  I,    1)  // add int
OP___(subi,     Op2,  I,    1)  // subtract int
OP___(muli,     Op2,  I,    1)  // multiply int
OP_86(divi,     Op2,  I,    1)  // divide int
// LIR_modi is a hack.  It's only used on i386/X64.  The operand is the result
// of a LIR_divi because on i386/X64 div and mod results are computed by the
// same instruction.
OP_86(modi,     Op1,  I,    1)  // modulo int

OP___(noti,     Op1,  I,    1)  // bitwise-NOT int
OP___(andi,     Op2,  I,    1)  // bitwise-AND int
OP___(ori,      Op2,  I,    1)  // bitwise-OR int
OP___(xori,     Op2,  I,    1)  // bitwise-XOR int

// For all three integer shift operations, only the bottom five bits of the
// second operand are used, and they are treated as unsigned.  This matches
// x86 semantics.
OP___(lshi,     Op2,  I,    1)  // left shift int
OP___(rshi,     Op2,  I,    1)  // right shift int (>>)
OP___(rshui,    Op2,  I,    1)  // right shift unsigned int (>>>)

OP_64(addq,     Op2,  Q,    1)  // add quad
OP_64(subq,     Op2,  Q,    1)  // subtract quad

OP_64(andq,     Op2,  Q,    1)  // bitwise-AND quad
OP_64(orq,      Op2,  Q,    1)  // bitwise-OR quad
OP_64(xorq,     Op2,  Q,    1)  // bitwise-XOR quad

// For all three quad shift operations, only the bottom six bits of the
// second operand are used, and they are treated as unsigned.  This matches
// x86-64 semantics.
OP_64(lshq,     Op2,  Q,    1)  // left shift quad;           2nd operand is an int
OP_64(rshq,     Op2,  Q,    1)  // right shift quad;          2nd operand is an int
OP_64(rshuq,    Op2,  Q,    1)  // right shift unsigned quad; 2nd operand is an int

OP___(negd,     Op1,  D,    1)  // negate double
OP___(absd,     Op1,  D,    1)  // absolute value of double
OP___(sqrtd,    Op1,  D,    1)  // sqrt double
OP___(addd,     Op2,  D,    1)  // add double
OP___(subd,     Op2,  D,    1)  // subtract double
OP___(muld,     Op2,  D,    1)  // multiply double
OP___(divd,     Op2,  D,    1)  // divide double

// LIR_modd is just a place-holder opcode, ie. the back-ends cannot generate
// code for it.  It's used in TraceMonkey briefly but is always demoted to a
// LIR_modl or converted to a function call before Nanojit has to do anything
// serious with it.
OP___(modd,     Op2,  D,    1)  // modulo double

OP___(negf,     Op1,  F,    1)  // negate float
OP___(absf,     Op1,  F,    1)  // absolute value of float
OP___(sqrtf,    Op1,  F,    1)  // sqrt float
OP___(addf,     Op2,  F,    1)  // add float
OP___(subf,     Op2,  F,    1)  // subtract float
OP___(mulf,     Op2,  F,    1)  // multiply float
OP___(divf,     Op2,  F,    1)  // divide float

OP___(negf4,    Op1, F4,    1)  // negate float4
OP___(absf4,    Op1, F4,    1)  // absolute value of float4
OP___(sqrtf4,   Op1, F4,    1)  // sqrt float4
OP___(addf4,    Op2, F4,    1)  // add float4
OP___(subf4,    Op2, F4,    1)  // subtract float4
OP___(mulf4,    Op2, F4,    1)  // multiply float4
OP___(divf4,    Op2, F4,    1)  // divide float4

OP___(recipf,   Op1,  F,    1)  // float reciprocal
OP___(rsqrtf,   Op1,  F,    1)  // float reciprocal square root
OP___(minf,     Op2,  F,    1)  // float min
OP___(maxf,     Op2,  F,    1)  // float max

OP___(cmpgtf4,  Op2, F4,    1)  // float4.isGreater
OP___(cmpltf4,  Op2, F4,    1)  // float4.isLess
OP___(cmpgef4,  Op2, F4,    1)  // float4.isGreaterOrEqual
OP___(cmplef4,  Op2, F4,    1)  // float4.isLessOrEqual
OP___(cmpeqf4,  Op2, F4,    1)  // float4.isEqual
OP___(cmpnef4,  Op2, F4,    1)  // float4.isNotEqual
OP___(recipf4,  Op1, F4,    1)  // float4 reciprocal
OP___(rsqrtf4,  Op1, F4,    1)  // float4 reciprocal square root
OP___(minf4,    Op2, F4,    1)  // float4 min
OP___(maxf4,    Op2, F4,    1)  // float4 max

OP___(dotf4,    Op2,  F,    1)  // 4-component dot product
OP___(dotf3,    Op2,  F,    1)  // 3-component dot product
OP___(dotf2,    Op2,  F,    1)  // 2-component dot product

OP___(cmovi,    Op3,  I,    1)  // conditional move int
OP_64(cmovq,    Op3,  Q,    1)  // conditional move quad
OP___(cmovd,    Op3,  D,    1)  // conditional move double
OP___(cmovf,    Op3,  F,    1)  // conditional move float
OP___(cmovf4,   Op3, F4,    1)  // conditional move float4

//---------------------------------------------------------------------------
// Conversions
//---------------------------------------------------------------------------
OP_64(i2q,      Op1,  Q,    1)  // sign-extend int to quad
OP_64(ui2uq,    Op1,  Q,    1)  // zero-extend unsigned int to unsigned quad
OP_64(q2i,      Op1,  I,    1)  // truncate quad to int (removes the high 32 bits)
OP_64(q2d,      Op1,  D,    1)  // convert quad to double

OP___(i2d,      Op1,  D,    1)  // convert int to double
OP___(i2f,      Op1,  F,    1)  // convert int to float
OP___(ui2d,     Op1,  D,    1)  // convert unsigned int to double
OP___(ui2f,     Op1,  F,    1)  // convert unsigned int to float

OP___(f2d,      Op1,  D,    1)  // convert float to double

// rounding behavior of LIR_d2f is platform-specific
//
// Platform     Asm code        Behavior
// --------     --------        --------
// x86 w/ x87   FST32           uses current FP control word (default is rounding)
// x86 w/ SSE   cvtsd2ss        according to MXCSR register (default is round to nearest)
// x64 (SSE)    cvtsd2ss        according to MXCSR register (default is round to nearest)
// others       not implemented yet
OP___(d2f,      Op1,  F,    1)  // convert double to float (no exceptions raised)

// The rounding behavior of LIR_d2i is platform specific.
//
// Platform     Asm code        Behavior
// --------     --------        --------
// x86 w/ x87   fist            uses current FP control word (default is rounding)
// x86 w/ SSE   cvttsd2si       performs round to zero (truncate)
// x64 (SSE)    cvttsd2si       performs round to zero (truncate) 
// PowerPC                      unsupported
// ARM          ftosid          round to nearest
// MIPS         trunc.w.d       performs round to zero (truncate)
// SH4          frtc            performs round to zero (truncate)
// SPARC        fdtoi           performs round to zero (truncate)
//
// round to zero examples:  1.9 -> 1, 1.1 -> 1, -1.1 -> -1, -1.9 -> -1
// round to nearest examples: 1.9 -> 2, 1.1 -> 1, -1.1 -> -1, -1.9 -> -2
OP___(d2i,      Op1,  I,    1)  // convert double to int (no exceptions raised)
OP___(f2i,      Op1,  I,    1)  // convert float to int (no exceptions raised)

OP___(f2f4,     Op1, F4,    1)  // convert float to float4 (no exceptions raised) - essentially copies the float across all elements
OP___(ffff2f4,  Op4, F4,    1)  // convert float to float4 (no exceptions raised) - essentially copies the float across all elements
OP___(f4x,      Op1,  F,    1)  // extract first float from a float4 
OP___(f4y,      Op1,  F,    1)  // extract second float from a float4 
OP___(f4z,      Op1,  F,    1)  // extract third float from a float4 
OP___(f4w,      Op1,  F,    1)  // extract fourth float from a float4 
OP___(swzf4,    Op1b,F4,    1)  // swizzle float4 according to 8-bit selector

OP_64(dasq,     Op1,  Q,    1)  // interpret the bits of a double as a quad
OP_64(qasd,     Op1,  D,    1)  // interpret the bits of a quad as a double

//---------------------------------------------------------------------------
// Overflow arithmetic
//---------------------------------------------------------------------------
// These all exit if overflow occurred.  The result is valid on either path.
OP___(addxovi,  Op3,  I,    1)  // add int and exit on overflow
OP___(subxovi,  Op3,  I,    1)  // subtract int and exit on overflow
OP___(mulxovi,  Op3,  I,    1)  // multiply int and exit on overflow

// These all branch if overflow occurred.  The result is valid on either path.
OP___(addjovi,  Op3,  I,    1)  // add int and branch on overflow
OP___(subjovi,  Op3,  I,    1)  // subtract int and branch on overflow
OP___(muljovi,  Op3,  I,    1)  // multiply int and branch on overflow

OP_64(addjovq,  Op3,  Q,    1)  // add quad and branch on overflow
OP_64(subjovq,  Op3,  Q,    1)  // subtract quad and branch on overflow

//---------------------------------------------------------------------------
// SoftFloat
//---------------------------------------------------------------------------
OP_SF(dlo2i,    Op1,  I,    1)  // get the low  32 bits of a double as an int
OP_SF(dhi2i,    Op1,  I,    1)  // get the high 32 bits of a double as an int
OP_SF(ii2d,     Op2,  D,    1)  // join two ints (1st arg is low bits, 2nd is high)

// LIR_hcalli is a hack that's only used on 32-bit platforms that use
// SoftFloat.  Its operand is always a LIR_calli, but one that specifies a
// function that returns a double.  It indicates that the double result is
// returned via two 32-bit integer registers.  The result is always used as the
// second operand of a LIR_ii2d.
OP_SF(hcalli,   Op1,  I,    1)

//---------------------------------------------------------------------------
// Safepoint Polling
//---------------------------------------------------------------------------
OP___(memfence, Op0, V, 0)
OP___(brsavpc, Op2, V, 0)        // branch and save pc
OP___(restorepc, Op0, V, 0)
OP___(pushstate, Op0, V, 0)
OP___(popstate, Op0, V, 0)
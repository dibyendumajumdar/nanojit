#ifndef __nanojit_extra__
#define __nanojit_extra__

#include <stdint.h>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NJXLIns *NJXLInsRef;

/**
* The Jit Context defines a container for the Jit machinery and
* also acts as the repository of the compiled functions. The Jit
* Context must be kept alive as long as any functions within it are
* needed. Deleting the Jit Context will also delete all compiled
* functions managed by the context.
*/
typedef struct NJXContext *NJXContextRef;

typedef struct NFXFunctionBuilder *NJXFunctionBuilderRef;

typedef int64_t NJXParamType;

/**
* Creates a Jit Context. If versbose is true then during code
* generation verbose output is generated.
*/
extern NJXContextRef NJX_create_context(int verbose);

/**
* Destroys the Jit Context. Note that all compiled functions
* managed by this context will die at this point.
*/
extern void NJX_destroy_context(NJXContextRef);

/**
* Returns a compiled function looking it up by name.
* The point must be cast to the correct signature.
*/
extern void *NJX_get_function_by_name(const char *name);

/**
* Creates a new FunctionBuilder object. The builder is used to construct the
* code that will go into one function. Once the function has been defined,
* it can be compiled by calling finalize(). After the function is compiled the
* builder object can be thrown away - the compiled function will live as long as
* the
* owning Jit Context lives.
* If optimize flag is true then NanoJit's CSE and Expr filters are enabled.
*/
extern NJXFunctionBuilderRef NJX_create_function_builder(NJXContextRef context,
                                                  const char *name,
                                                  int optimize);

/**
* Destroys the FunctionBuilder object. Note that this will not delete the
* compiled function created using this builder - as the compiled function lives
* in the owning Jit Context.
*/
extern void NJX_destroy_function_builder(NJXFunctionBuilderRef);

/**
* Adds an integer return instruction.
*/
extern NJXLInsRef NJX_reti(NJXFunctionBuilderRef fn, NJXLInsRef result);

/**
* Adds a double return instruction.
*/
extern NJXLInsRef NJX_retd(NJXFunctionBuilderRef fn, NJXLInsRef result);

/**
* Adds a quad return instruction.
*/
extern NJXLInsRef NJX_retq(NJXFunctionBuilderRef fn, NJXLInsRef result);

/**
* Creates an int32 constant
*/
extern NJXLInsRef NJX_immi(NJXFunctionBuilderRef fn, int32_t i);

/**
* Creates an int64 constant
*/
extern NJXLInsRef NJX_immq(NJXFunctionBuilderRef fn, int64_t q);

/**
* Creates a double constant
*/
extern NJXLInsRef NJX_immd(NJXFunctionBuilderRef fn, double d);

/**
* Adds a function parameter - the parameter size is always the
* default register size I think - so on a 64-bit machine it will be
* quads whereas on 32-bit machines it will be words. Caller must
* handle this and convert to type needed.
* This also means that only primitive values and pointers can be
* used as function parameters.
*/
extern NJXLInsRef NJX_insert_parameter(NJXFunctionBuilderRef fn);

/**
* Integer add
*/
extern NJXLInsRef NJX_addi(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs);

/**
* Converts a quad to an int
*/
extern NJXLInsRef NJX_q2i(NJXFunctionBuilderRef fn, NJXLInsRef q);

/**
* Completes the function, and assembles the code.
* If assembly is successful then the generated code is saved in the parent
* Context object
* by fragment name. The pointer to executable function is returned. Note that
* the pointer is
* valid only until the NanoJitContext is valid, as all functions are destroyed
* when the
* Context ends.
*/
extern void *NJX_finalize(NJXFunctionBuilderRef fn);

#ifdef __cplusplus
}
#endif

#endif
/**
* The examples here illustrate how to use the
* NanoJITExtra C API.
*/
#include <nanojitextra.h>

#include <stdint.h>

#include <iostream>
#include <map>
#include <string>

/**
* Compiles simplest function:
* int simplest(void) { return 0; }
*/
int simplest(NJXContextRef jit) {
  const char *name = "simplest";
  typedef int (*functype)(void);

  // Create a function builder
  NJXFunctionBuilderRef builder =
      NJX_create_function_builder(jit, name, NJXValueKind_I, nullptr, 0, true);

  auto zero = NJX_immi(builder, 0);
  auto ret = NJX_reti(builder, zero);

  functype f = (functype)NJX_finalize(builder);

  NJX_destroy_function_builder(builder);

  if (f != nullptr)
    return f();
  return 1;
}

/**
* Compiles a function that adds 2 to given int value and returns result
* int add2(int x) { return x+2; }
*/
int add2(NJXContextRef jit) {
  const char *name = "add2";
  typedef int (*functype)(NJXParamType);

  // Create a function builder
  NJXValueKind args[1] = {NJXValueKind_I};
  NJXFunctionBuilderRef builder =
      NJX_create_function_builder(jit, name, NJXValueKind_I, args, 1, true);

  auto two = NJX_immi(builder, 2);              /* 2 */
  auto param1 = NJX_get_parameter(builder, 0);  /* arg1 */
  auto result = NJX_addi(builder, param1, two); /* add */
  auto ret = NJX_reti(builder, result);         /* return result */

  functype f = (functype)NJX_finalize(builder);

  NJX_destroy_function_builder(builder);

  if (f != nullptr)
    return f(5) == 7 ? 0 : 1;
  return 1;
}

/**
* Tests a simple add function that takes two int values and returns their sum
* int add(int x, int y) { return x+y; }
*/
int add(NJXContextRef jit) {
  const char *name = "add";
  typedef int (*functype)(NJXParamType, NJXParamType);

  // Create a function builder
  NJXValueKind args[2] = {NJXValueKind_I, NJXValueKind_I};
  NJXFunctionBuilderRef builder =
      NJX_create_function_builder(jit, name, NJXValueKind_I, args, 2, true);

  auto x = NJX_get_parameter(builder, 0); /* arg1 */
  auto y = NJX_get_parameter(builder, 1); /* arg2 */
  auto result = NJX_addi(builder, x, y);  /* result = x + y */
  auto ret = NJX_reti(builder, result);   /* return result */

  functype f = (functype)NJX_finalize(builder);

  NJX_destroy_function_builder(builder);

  if (f != nullptr)
    return f(100, 200) == 300 ? 0 : 1;
  return 1;
}

/**
* This example test a simple function call.
* First we have an add funcion:
* int add(int x, int y) { return x+y; }
* Next we create a function that calls add().
* int caller() { return add(100, 200); }
*/
int calladd(NJXContextRef jit) {
  typedef int (*adderfunc)(NJXParamType, NJXParamType);
  typedef int (*callerfunc)();

  // Create the add function
  // int add(int x, int y) { return x+y; }
  NJXValueKind args1[2] = {NJXValueKind_I, NJXValueKind_I};
  NJXFunctionBuilderRef builder =
      NJX_create_function_builder(jit, "add", NJXValueKind_I, args1, 2, true);
  auto x = NJX_get_parameter(builder, 0); /* arg1 */
  auto y = NJX_get_parameter(builder, 1); /* arg2 */
  auto result = NJX_addi(builder, x, y);  /* result = x + y */
  auto ret = NJX_reti(builder, result);   /* return result */
  adderfunc fadd = (adderfunc)NJX_finalize(builder);
  NJX_destroy_function_builder(builder);

  // Create the caller function
  // int caller() { return add(100, 200); }
  builder = NJX_create_function_builder(jit, "caller", NJXValueKind_I, nullptr,
                                        0, true);
  x = NJX_immi(builder, 100);
  y = NJX_immi(builder, 200);
  NJXLInsRef args[2] = {x, y};
  result =
      NJX_calli(builder, "add", NJXCallAbiKind::NJX_CALLABI_FASTCALL, 2, args);
  ret = NJX_reti(builder, result);
  callerfunc fcaller = (callerfunc)NJX_finalize(builder);
  NJX_destroy_function_builder(builder);

  if (fadd != nullptr && fcaller != nullptr)
    return fcaller() == 300 ? 0 : 1;
  return 1;
}

/**
* int64 multiply function
* int64_t mult(int64_t x, int64_t y) { return (x*y)*8LL; }
*/
int mult(NJXContextRef jit) {
  const char *name = "mult";
  typedef int64_t (*functype)(NJXParamType, NJXParamType);

  // Create a function builder
  NJXValueKind args[2] = {NJXValueKind_Q, NJXValueKind_Q};
  NJXFunctionBuilderRef builder =
      NJX_create_function_builder(jit, name, NJXValueKind_Q, args, 2, true);

  auto x = NJX_get_parameter(builder, 0); /* arg1 */
  auto y = NJX_get_parameter(builder, 1); /* arg2 */
  auto z = NJX_mulq(builder, x, y);       /* z = x * y */
  // The multiplication where the RHS is an int32 is special cased
  // so this is an attempt to validate that this works correctly
  auto imm8 = NJX_immq(builder, 8);
  auto result = NJX_mulq(builder, z, imm8);
  auto ret = NJX_retq(builder, result); /* return result */

  functype f = (functype)NJX_finalize(builder);

  NJX_destroy_function_builder(builder);

  if (f != nullptr) {
    return f(100, 200) == 160000 ? 0 : 1;
  }
  return 1;
}

/**
* int64 div function
* int64_t div(int64_t x, int64_t y) { return x/y; }
*/
int div(NJXContextRef jit) {
  const char *name = "div";
  typedef int64_t (*functype)(NJXParamType, NJXParamType);

  // Create a function builder
  NJXValueKind args[2] = {NJXValueKind_Q, NJXValueKind_Q};
  NJXFunctionBuilderRef builder =
      NJX_create_function_builder(jit, name, NJXValueKind_Q, args, 2, true);

  auto x = NJX_get_parameter(builder, 0); /* arg1 */
  auto y = NJX_get_parameter(builder, 1); /* arg2 */
  auto result = NJX_divq(builder, x, y);  /* result = x / y */
  auto ret = NJX_retq(builder, result);   /* return result */

  functype f = (functype)NJX_finalize(builder);

  NJX_destroy_function_builder(builder);

  if (f != nullptr)
    return f(250, 100) == 2 ? 0 : 1;
  return 1;
}

int main(int argc, const char *argv[]) {

  NJXContextRef jit = NJX_create_context(true);

  int rc = 0;
  rc += simplest(jit);
  rc += add2(jit);
  rc += add(jit);
  rc += mult(jit);
  rc += div(jit);
  rc += calladd(jit);

  NJX_destroy_context(jit);

  if (rc == 0)
    printf("Test OK\n");
  else
    printf("Test FAILED\n");
  return rc;
}

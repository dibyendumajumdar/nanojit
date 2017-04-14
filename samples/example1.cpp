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
  NJXFunctionBuilderRef builder = NJX_create_function_builder(jit, name, true);

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
  NJXFunctionBuilderRef builder = NJX_create_function_builder(jit, name, true);

  auto two = NJX_immi(builder, 2);              /* 2 */
  auto param1 = NJX_insert_parameter(builder);  /* arg1 */
  param1 = NJX_q2i(builder, param1);            /* (int) arg1 */
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
*/
int add(NJXContextRef jit) {
  const char *name = "add";
  typedef int (*functype)(NJXParamType, NJXParamType);

  // Create a function builder
  NJXFunctionBuilderRef builder = NJX_create_function_builder(jit, name, true);

  auto param1 = NJX_insert_parameter(builder); /* arg1 */
  auto param2 = NJX_insert_parameter(builder); /* arg2 */
  auto x = NJX_q2i(builder, param1);           /* x = (int) arg1 */
  auto y = NJX_q2i(builder, param2);           /* y = (int) arg2 */
  auto result = NJX_addi(builder, x, y);       /* result = x = y */
  auto ret = NJX_reti(builder, result);        /* return result */

  functype f = (functype)NJX_finalize(builder);

  NJX_destroy_function_builder(builder);

  if (f != nullptr)
    return f(100, 200) == 300 ? 0 : 1;
  return 1;
}

int calladd(NJXContextRef jit) {
  typedef int (*adderfunc)(NJXParamType, NJXParamType);
  typedef int (*callerfunc)();

  // Create the add function
  // int add(int x, int y) { return x+y; }
  NJXFunctionBuilderRef builder = NJX_create_function_builder(jit, "add", true);
  auto param1 = NJX_insert_parameter(builder); /* arg1 */
  auto param2 = NJX_insert_parameter(builder); /* arg2 */
  auto x = NJX_q2i(builder, param1);           /* x = (int) arg1 */
  auto y = NJX_q2i(builder, param2);           /* y = (int) arg2 */
  auto result = NJX_addi(builder, x, y);       /* result = x = y */
  auto ret = NJX_reti(builder, result);        /* return result */
  adderfunc fadd = (adderfunc)NJX_finalize(builder);
  NJX_destroy_function_builder(builder);

  // Create the caller function
  // int caller() { return add(100, 200); }
  builder = NJX_create_function_builder(jit, "caller", true);
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

int main(int argc, const char *argv[]) {

  NJXContextRef jit = NJX_create_context(true);

  int rc = 0;
  rc += simplest(jit);
  rc += add2(jit);
  rc += add(jit);
  rc += calladd(jit);

  NJX_destroy_context(jit);

  return rc;
}

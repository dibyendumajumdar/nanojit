#include <nanojitextra.h>

#include <stdint.h>

#include <string>
#include <map>
#include <iostream>

using namespace nanojit;


/**
* Compiles simplest function:
* int simplest(void) { return 0; }
*/
int simplest(NanoJitContext &jit)
{
	std::string name = "simplest";
	typedef int (*functype)(void);

	// Create a function builder
	FunctionBuilder builder(jit, name, true);

	auto zero = builder.immi(0);
	auto ret = builder.reti(zero);

	functype f = (functype) builder.finalize();

	if (f != nullptr)
		return f();
	return 1;
}

/**
* Compiles a function that adds 2 to given int value and returns result
* int add2(int x) { return x+2; }
*/
int add2(NanoJitContext  &jit)
{
	std::string name = "add2";
	typedef int(*functype)(parameter_type);

	// Create a function builder
	FunctionBuilder builder(jit, name, true);

	auto two = builder.immi(2);                /* 2 */
	auto param1 = builder.insertParameter();   /* arg1 */
	param1 = builder.q2i(param1);              /* (int) arg1 */
	auto result = builder.addi(param1, two);   /* add */
	auto ret = builder.reti(result);           /* return result */

	functype f = (functype)builder.finalize();

	if (f != nullptr)
		return f(5) == 7 ? 0 : 1;
	return 1;
}

/**
* Tests a simple add function that takes two int values and returns their sum
*/
int add(NanoJitContext &jit)
{
	std::string name = "add";
	typedef int(*functype)(parameter_type, parameter_type);

	// Create a function builder
	FunctionBuilder builder(jit, name, true);

	auto param1 = builder.insertParameter();   /* arg1 */
	auto param2 = builder.insertParameter();   /* arg2 */
	auto x = builder.q2i(param1);              /* x = (int) arg1 */
	auto y = builder.q2i(param2);              /* y = (int) arg2 */
	auto result = builder.addi(x, y);          /* result = x = y */
	auto ret = builder.reti(result);           /* return result */

	functype f = (functype)builder.finalize();

	if (f != nullptr)
		return f(100, 200) == 300 ? 0 : 1;
	return 1;
}


int main(int argc, const char *argv[]) {

	NanoJitContext jit(true);

	int rc = 0;
	rc += simplest(jit);
	rc += add2(jit);
	rc += add(jit);

	return rc;
}

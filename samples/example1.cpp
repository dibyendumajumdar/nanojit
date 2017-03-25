#include <nanojitextra.h>

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

	// Create a fragment assembler - we can treat a fragment as a function
	FunctionBuilder fragment(jit, name, true);

	auto zero = fragment.immi(0);
	auto ret = fragment.ret(RT_INT, zero);

	functype f = (functype) fragment.finalize();

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
	typedef int(*functype)(int);

	// Create a fragment assembler - we can treat a fragment as a function
	FunctionBuilder fragment(jit, name, true);

	auto two = fragment.immi(2); /* 2 */
	auto param1 = fragment.insertParameter(); /* arg1 */
	param1 = fragment.q2i(param1); /* (int) arg1 */
	auto result = fragment.addi(param1, two); /* add */
	auto ret = fragment.ret(RT_INT, result); /* return result */

	functype f = (functype)fragment.finalize();

	if (f != nullptr)
		return f(5) == 7 ? 0 : 1;
	return 1;
}


int main(int argc, const char *argv[]) {

	NanoJitContext jit(true);

	int rc = 0;
	rc += simplest(jit);
	rc += add2(jit);

	return rc;
}

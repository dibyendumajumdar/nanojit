#ifndef __nanojit_extra__
#define __nanojit_extra__

#include <nanojit.h>

#include <string>

namespace nanojit {

	class NanoJitContextImpl;
	class FunctionBuilderImpl;

	enum ReturnType {
		RT_INT = 1,
#ifdef NANOJIT_64BIT
		RT_QUAD = 2,
#endif
		RT_DOUBLE = 4,
	};

	class NanoJitContext {
	private:
		NanoJitContextImpl *impl_;
	public:
		NanoJitContext(bool verbose);
		~NanoJitContext();
		NanoJitContextImpl& impl() { return *impl_; }

		void *function_by_name(const std::string& name);
	};

	class FunctionBuilder {
	private:
		FunctionBuilderImpl *impl_;
	public:
		FunctionBuilder(NanoJitContext& context, const std::string& name, bool optimize);
		~FunctionBuilder();

		/**
		* Adds a ret instruction.
		*/
		LIns *ret(ReturnType rt, LIns *result);

		/**
		* Creates an int32 constant
		*/
		LIns *immi(int32_t i);

		/**
		* Creates an int64 constant
		*/
		LIns *immq(int64_t q);

		/**
		* Creates a double constant
		*/
		LIns *immd(double d);

		/**
		* Adds a function parameter - the parameter size is always the
		* default register size I think - so on a 64-bit machine it will be
		* quads whereas on 32-bit machines it will be words. Caller must
		* handle this and convert to type needed.
		* This also means that only primitive values and pointers can be
		* used as function parameters.
		*/
		LIns *insertParameter();

		/**
		* Integer add
		*/
		LIns *addi(LIns *lhs, LIns *rhs);

		/**
		* Converts a quad to an int
		*/
		LIns *q2i(LIns *q);

		/**
		* Completes the function, and assembles the code.
		* If assembly is successful then the generated code is saved in the parent Context object
		* by fragment name. The pointer to executable function is returned. Note that the pointer is
		* valid only until the NanoJitContext is valid, as all functions are destroyed when the
		* Context ends.
		*/
		void *finalize();
	};


}



#endif
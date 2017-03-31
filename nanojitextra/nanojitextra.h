#ifndef __nanojit_extra__
#define __nanojit_extra__

#include <nanojit.h>

#include <stdint.h>

#include <string>

#ifndef NANOJIT_64BIT
#error This code is only supported on 64-bit architecture
#endif

namespace nanojit {

	class NanoJitContextImpl;
	class FunctionBuilderImpl;

	typedef int64_t parameter_type;

	/**
	* The Jit Context defines a container for the Jit machinery and
	* also acts as the repository of the compiled functions. The Jit 
	* Context must be kept alive as long as any functions within it are
	* needed. Deleting the Jit Context will also delete all compiled 
	* functions managed by the context. 
	*/
	class NanoJitContext {
	private:
		NanoJitContextImpl *impl_;
	public:
		/**
		* Creates a Jit Context. If versbose is true then during code
		* generation verbose output is generated.
		*/
		NanoJitContext(bool verbose);

		/**
		* Destroys the Jit Context. Note that all compiled functions
		* managed by this context will die at this point.
		*/
		~NanoJitContext();

		/**
		* Returns the implementation - for internal use only.
		*/
		NanoJitContextImpl& impl() { return *impl_; }

		/**
		* Returns a compiled function looking it up by name.
		* The point must be cast to the correct signature.
		*/
		void *function_by_name(const std::string& name);
	};

	class FunctionBuilder {
	private:
		FunctionBuilderImpl *impl_;
	public:
		/**
		* Creates a new FunctionBuilder object. The builder is used to construct the
		* code that will go into one function. Once the function has been defined,
		* it can be compiled by calling finalize(). After the function is compiled the
		* builder object can be thrown away - the compiled function will live as long as the
		* owning Jit Context lives.
		* If optimize flag is true then NanoJit's CSE and Expr filters are enabled.
		*/
		FunctionBuilder(NanoJitContext& context, const std::string& name, bool optimize);

		/**
		* Destroys the FunctionBuilder object. Note that this will not delete the
		* compiled function created using this builder - as the compiled function lives
		* in the owning Jit Context.
		*/
		~FunctionBuilder();

		/**
		* Adds an integer return instruction.
		*/
		LIns *reti(LIns *result);

		/**
		* Adds a double return instruction.
		*/
		LIns *retd(LIns *result);

		/**
		* Adds a quad return instruction.
		*/
		LIns *retq(LIns *result);

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
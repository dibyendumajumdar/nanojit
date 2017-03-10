#include <nanojit.h>

using namespace nanojit;

class Jit {
private:

	/**
	* A struct used to configure the assumptions that Assembler can make when
	* generating code. The ctor will fill in all fields with the most reasonable
	* values it can derive from compiler flags and/or runtime detection, but
	* the embedder is free to override any or all of them as it sees fit.
	* Using the ctor-provided default setup is guaranteed to provide a safe
	* runtime environment (though perhaps suboptimal in some cases), so an embedder
	* should replace these values with great care.
	*
	* Note that although many fields are used on only specific architecture(s),
	* this struct is deliberately declared without ifdef's for them, so (say) ARM-specific
	* fields are declared everywhere. This reduces build dependencies (so that this
	* files does not require nanojit.h to be included beforehand) and also reduces
	* clutter in this file; the extra storage space required is trivial since most
	* fields are single bits.
	*/
	Config config_;

	/**
	* Allocator is a bump-pointer allocator with an SPI for getting more
	* memory from embedder-implemented allocator, such as malloc()/free().
	*
	* alloc() never returns NULL.  The implementation of allocChunk()
	* is expected to perform a longjmp or exception when an allocation can't
	* proceed.
	*/
	Allocator alloc_;

	/**
	* Code memory allocator is a long lived manager for many code blocks that
	* manages interaction with an underlying code memory allocator,
	* sets page permissions.  CodeAlloc provides APIs for allocating and freeing
	* individual blocks of code memory (for methods, stubs, or compiled
	* traces), static functions for managing lists of allocated code, and has
	* a few pure virtual methods that embedders must implement to provide
	* memory to the allocator.
	*
	* A "chunk" is a region of memory obtained from allocCodeChunk; it must
	* be page aligned and be a multiple of the system page size.
	*
	* A "block" is a region of memory within a chunk.  It can be arbitrarily
	* sized and aligned, but is always contained within a single chunk.
	* class CodeList represents one block; the members of CodeList track the
	* extent of the block and support creating lists of blocks.
	*
	* The allocator coalesces free blocks when it can, in free(), but never
	* coalesces chunks.
	*/
	CodeAlloc code_alloc_;
	bool verbose_;
	// Fragments mFragments;
	Assembler asm_;

	// LogControl, a class for controlling and routing debug output
	LogControl logc_;


public:
	Jit(bool verbose, Config& config);
	~Jit();

};

Jit::Jit(bool verbose, Config& config) :
	config_(config),
	code_alloc_(&config),
	asm_(code_alloc_, alloc_, alloc_, &logc_, config_)
{
}

Jit::~Jit() {

}


int main(int argc, const char *argv[]) {

	Config config;
	Jit jit(true, config);


	return 0;
}

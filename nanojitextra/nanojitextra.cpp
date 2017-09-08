#include <nanojit.h>
#include <nanojitextra.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#ifndef NANOJIT_64BIT
#error This code is only supported on 64-bit architecture
#endif

namespace nanojit {

enum ReturnType {
  RT_INT = 1,
  RT_QUAD = 2,
  RT_DOUBLE = 4,
  RT_FLOAT = 8,
};

// We lump everything into a single access region for lirasm.
static const AccSet ACCSET_OTHER = (1 << 0);
static const uint8_t LIRASM_NUM_USED_ACCS = 1;

typedef int32_t(FASTCALL *RetInt)();
typedef int64_t(FASTCALL *RetQuad)();
typedef double(FASTCALL *RetDouble)();
typedef float(FASTCALL *RetFloat)();

struct Function {
  std::string name;
  struct nanojit::CallInfo callInfo;
};

class LirasmFragment {
public:
  union {
    RetInt rint;
    RetQuad rquad;
    RetDouble rdouble;
    RetFloat rfloat;
  };
  ReturnType mReturnType;
  Fragment *fragptr;
  uint32_t typeSig;
};

typedef std::map<std::string, LirasmFragment> Fragments;
typedef std::vector<Function> Functions;

// Equivalent to Lirasm
class NanoJitContextImpl {
public:
  bool verbose_;
  /**
  * A struct used to configure the assumptions that Assembler can make when
  * generating code. The ctor will fill in all fields with the most reasonable
  * values it can derive from compiler flags and/or runtime detection, but
  * the embedder is free to override any or all of them as it sees fit.
  * Using the ctor-provided default setup is guaranteed to provide a safe
  * runtime environment (though perhaps suboptimal in some cases), so an
  * embedder
  * should replace these values with great care.
  *
  * Note that although many fields are used on only specific architecture(s),
  * this struct is deliberately declared without ifdef's for them, so (say)
  * ARM-specific
  * fields are declared everywhere. This reduces build dependencies (so that
  * this
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

  /**
  * All compiled fragments are saved in a map by fragment name
  */
  Fragments fragments_;

  Assembler asm_;

  /**
  * LirBuffer object to hold LIR instructions
  */
  LirBuffer *lirbuf_;

  // LogControl, a class for controlling and routing debug output
  LogControl logc_;

  Functions external_functions_;

public:
  NanoJitContextImpl(bool verbose, Config config);
  ~NanoJitContextImpl();

  LirasmFragment *get_fragment(const char *name);

  // Lookup a function in fragments; populate CallInfo if found
  // Returns 0 if not found
  // Returns 1 if external
  // Returns 2 if internal
  int lookupFunction(const std::string &name, CallInfo *&ci);

  // Register an external function - assumed to be C calling
  // convention
  bool registerFunction(const std::string &name, void *fptr, ArgType retval,
                        const ArgType *args, int argc);
};

/**
* Assembles a fragment - the fragment is saved in the parent Jit object by name.
* A fragment can be thought of as a function, at least that is how we use it
* for now.
*/
class FunctionBuilderImpl {
private:
  NanoJitContextImpl &parent_;

  const std::string fragName_;

  /**
  * Once the instructions are in the LirBuffer, the application calls
  * parent_.asm_.compile() to produce machine code, which is stored in
  * the fragment. The result of compilation is a function that the
  * application can call from C via a pointer to the first instruction.
  */
  Fragment *fragment_;

  bool optimize_;

  LirWriter *lir_;

  /**
  * LirBufWriter object to write instructions to the buffer
  */
  LirBufWriter *bufWriter_;

  /*
  * The LirBufWriter is wrapped in zero or more other LirWriter objects,
  * all of which implement the same interface as LirBufWriter. This chain of
  * LirWriter objects forms a pipeline for the instructions to pass through.
  * Each LirWriter can perform an optimization or other task on the program
  * as it passes through the system and into the LirBuffer.
  */

  LirWriter *cseFilter_;

  LirWriter *exprFilter_;

  LirWriter *verboseWriter_;

  LirWriter *validateWriter1_;

  LirWriter *validateWriter2_;

  char returnTypeBits_;

  int32_t paramCount_;

  ArgType rvalue_;

  ArgType args_[MAXARGS];

  LIns *params_[MAXARGS];

private:
  static uint32_t sProfId;

public:
  FunctionBuilderImpl(NanoJitContextImpl &parent,
                      const std::string &fragmentName, ArgType rvalue,
                      const ArgType *args, int argc, bool optimize);
  ~FunctionBuilderImpl();

  /**
  * Adds an integer return instruction.
  */
  LIns *reti(LIns *result);

  /**
  * Adds a double return instruction.
  */
  LIns *retd(LIns *result);

  /**
  * Adds a float return instruction.
  */
  LIns *retf(LIns *result);

  /**
  * Adds a quad return instruction.
  */
  LIns *retq(LIns *result);

  /**
  * Add a void return - TODO check that LIR_x is the right instruction to emit
  */
  LIns *ret() { return lir_->ins0(LIR_x); }

  /**
  * Creates an int32 constant
  */
  LIns *immi(int32_t i) { return lir_->insImmI(i); }

  /**
  * Creates an int64 constant
  */
  LIns *immq(int64_t q) { return lir_->insImmQ(q); }

  /**
  * Creates a double constant
  */
  LIns *immd(double d) { return lir_->insImmD(d); }

  /**
  * Creates a float constant
  */
  LIns *immf(float f) { return lir_->insImmF(f); }

  /**
  * Adds a function parameter - the parameter size is always the
  * default register size I think - so on a 64-bit machine it will be
  * quads whereas on 32-bit machines it will be words. Caller must
  * handle this and convert to type needed.
  * This also means that only primitive values and pointers can be
  * used as function parameters.
  */
  LIns *insertParameter() { return lir_->insParam(paramCount_++, 0); }

  LIns *getParameter(int pos);

  /**
  * Insert a label at current position
  */
  LIns *addLabel() { return lir_->ins0(LIR_label); }

  /**
  * Allocate size bytes on the stack
  */
  LIns *allocA(int32_t size) { return lir_->insAlloc(size); }

  /**
  * Inserts an unconditional jump - to can be NULL and set later
  */
  LIns *br(LIns *to) { return lir_->insBranch(LIR_j, NULL, to); }

  /**
  * Inserts a conditional branch - jump targets can be NULL and set later
  */
  LIns *cbrTrue(LIns *cond, LIns *to) {
    return lir_->insBranch(LIR_jt, cond, to);
  }
  LIns *cbrFalse(LIns *cond, LIns *to) {
    return lir_->insBranch(LIR_jf, cond, to);
  }
  LIns *jmpTable(LIns *index, uint32_t size) {
    return lir_->insJtbl(index, size);
  }
  LIns *choose(LIns *cond, LIns *iftrue, LIns *iffalse, bool use_cmov) {
    return lir_->insChoose(cond, iftrue, iffalse, use_cmov);
  }

  LIns *loadc2i(LIns *ptr, int32_t offset) {
    return lir_->insLoad(LIR_ldc2i, ptr, offset, ACCSET_OTHER);
  }
  LIns *loaduc2ui(LIns *ptr, int32_t offset) {
    return lir_->insLoad(LIR_lduc2ui, ptr, offset, ACCSET_OTHER);
  }
  LIns *loads2i(LIns *ptr, int32_t offset) {
    return lir_->insLoad(LIR_lds2i, ptr, offset, ACCSET_OTHER);
  }
  LIns *loadus2ui(LIns *ptr, int32_t offset) {
    return lir_->insLoad(LIR_ldus2ui, ptr, offset, ACCSET_OTHER);
  }
  LIns *loadi(LIns *ptr, int32_t offset) {
    return lir_->insLoad(LIR_ldi, ptr, offset, ACCSET_OTHER);
  }
  LIns *loadq(LIns *ptr, int32_t offset) {
    return lir_->insLoad(LIR_ldq, ptr, offset, ACCSET_OTHER);
  }
  LIns *loadf(LIns *ptr, int32_t offset) {
    return lir_->insLoad(LIR_ldf, ptr, offset, ACCSET_OTHER);
  }
  LIns *loadd(LIns *ptr, int32_t offset) {
    return lir_->insLoad(LIR_ldd, ptr, offset, ACCSET_OTHER);
  }
  LIns *loadf2d(LIns *ptr, int32_t offset) {
    return lir_->insLoad(LIR_ldf2d, ptr, offset, ACCSET_OTHER);
  }

  LIns *storei2c(LIns *value, LIns *ptr, int32_t offset) {
    return lir_->insStore(LIR_sti2c, value, ptr, offset, ACCSET_OTHER);
  }
  LIns *storei2s(LIns *value, LIns *ptr, int32_t offset) {
    return lir_->insStore(LIR_sti2s, value, ptr, offset, ACCSET_OTHER);
  }
  LIns *storei(LIns *value, LIns *ptr, int32_t offset) {
    return lir_->insStore(LIR_sti, value, ptr, offset, ACCSET_OTHER);
  }
  LIns *storeq(LIns *value, LIns *ptr, int32_t offset) {
    return lir_->insStore(LIR_stq, value, ptr, offset, ACCSET_OTHER);
  }
  LIns *stored(LIns *value, LIns *ptr, int32_t offset) {
    return lir_->insStore(LIR_std, value, ptr, offset, ACCSET_OTHER);
  }
  LIns *storef(LIns *value, LIns *ptr, int32_t offset) {
    return lir_->insStore(LIR_stf, value, ptr, offset, ACCSET_OTHER);
  }

  LIns *addi(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_addi, lhs, rhs); }
  LIns *addq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_addq, lhs, rhs); }
  LIns *addd(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_addd, lhs, rhs); }
  LIns *addf(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_addf, lhs, rhs); }

  LIns *subi(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_subi, lhs, rhs); }
  LIns *subq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_subq, lhs, rhs); }
  LIns *subd(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_subd, lhs, rhs); }
  LIns *subf(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_subf, lhs, rhs); }

  LIns *muli(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_muli, lhs, rhs); }
  LIns *mulq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_mulq, lhs, rhs); }
  LIns *muld(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_muld, lhs, rhs); }
  LIns *mulf(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_mulf, lhs, rhs); }

  LIns *divi(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_divi, lhs, rhs); }
  LIns *divq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_divq, lhs, rhs); }
  LIns *divd(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_divd, lhs, rhs); }
  LIns *divf(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_divf, lhs, rhs); }

  LIns *modi(LIns *lhs, LIns *rhs) { return lir_->ins1(LIR_modi, lir_->ins2(LIR_divi, lhs, rhs)); }
  LIns *modq(LIns *lhs, LIns *rhs) { return lir_->ins1(LIR_modq, lir_->ins2(LIR_divq, lhs, rhs)); }

  LIns *eqi(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_eqi, lhs, rhs); }
  LIns *eqq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_eqq, lhs, rhs); }
  LIns *eqd(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_eqd, lhs, rhs); }
  LIns *eqf(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_eqf, lhs, rhs); }

  LIns *negi(LIns *lhs) { return lir_->ins1(LIR_negi, lhs); }
  LIns *negq(LIns *lhs) { return lir_->ins1(LIR_negq, lhs); }
  LIns *negf(LIns *lhs) { return lir_->ins1(LIR_negf, lhs); }
  LIns *negd(LIns *lhs) { return lir_->ins1(LIR_negd, lhs); }

  LIns *noti(LIns *lhs) { return lir_->ins1(LIR_noti, lhs); }
  LIns *notq(LIns *lhs) { return lir_->ins1(LIR_notq, lhs); }

  LIns *andi(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_andi, lhs, rhs); }
  LIns *andq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_andq, lhs, rhs); }

  LIns *ori(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_ori, lhs, rhs); }
  LIns *orq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_orq, lhs, rhs); }

  LIns *xori(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_xori, lhs, rhs); }
  LIns *xorq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_xorq, lhs, rhs); }

  LIns *lshi(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_lshi, lhs, rhs); }
  LIns *lshq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_lshq, lhs, rhs); }

  LIns *rshi(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_rshi, lhs, rhs); }
  LIns *rshq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_rshq, lhs, rhs); }

  LIns *rshui(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_rshui, lhs, rhs); }
  LIns *rshuq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_rshuq, lhs, rhs); }

  LIns *lti(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_lti, lhs, rhs); }
  LIns *lei(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_lei, lhs, rhs); }
  LIns *ltui(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_ltui, lhs, rhs); }
  LIns *leui(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_leui, lhs, rhs); }
  LIns *ltq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_ltq, lhs, rhs); }
  LIns *leq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_leq, lhs, rhs); }
  LIns *ltuq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_ltuq, lhs, rhs); }
  LIns *leuq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_leuq, lhs, rhs); }
  LIns *ltd(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_ltd, lhs, rhs); }
  LIns *led(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_led, lhs, rhs); }
  LIns *ltf(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_ltf, lhs, rhs); }
  LIns *lef(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_lef, lhs, rhs); }

  LIns *gti(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_gti, lhs, rhs); }
  LIns *gei(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_gei, lhs, rhs); }
  LIns *gtui(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_gtui, lhs, rhs); }
  LIns *geui(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_geui, lhs, rhs); }
  LIns *gtq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_gtq, lhs, rhs); }
  LIns *geq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_geq, lhs, rhs); }
  LIns *gtuq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_gtuq, lhs, rhs); }
  LIns *geuq(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_geuq, lhs, rhs); }
  LIns *gtd(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_gtd, lhs, rhs); }
  LIns *ged(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_ged, lhs, rhs); }
  LIns *gtf(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_gtf, lhs, rhs); }
  LIns *gef(LIns *lhs, LIns *rhs) { return lir_->ins2(LIR_gef, lhs, rhs); }

  LIns *i2q(LIns *q) { return lir_->ins1(LIR_i2q, q); }
  LIns *ui2uq(LIns *q) { return lir_->ins1(LIR_ui2uq, q); }
  LIns *q2i(LIns *q) { return lir_->ins1(LIR_q2i, q); }
  LIns *q2d(LIns *q) { return lir_->ins1(LIR_q2d, q); }
  LIns *i2d(LIns *q) { return lir_->ins1(LIR_i2d, q); }
  LIns *i2f(LIns *q) { return lir_->ins1(LIR_i2f, q); }
  LIns *ui2d(LIns *q) { return lir_->ins1(LIR_ui2d, q); }
  LIns *ui2f(LIns *q) { return lir_->ins1(LIR_ui2f, q); }
  LIns *f2d(LIns *q) { return lir_->ins1(LIR_f2d, q); }
  LIns *d2f(LIns *q) { return lir_->ins1(LIR_d2f, q); }
  LIns *d2i(LIns *q) { return lir_->ins1(LIR_d2i, q); }
  LIns *f2i(LIns *q) { return lir_->ins1(LIR_f2i, q); }
  LIns *d2q(LIns *q) { return lir_->ins1(LIR_d2q, q); }
  LIns *qasd(LIns *q) { return lir_->ins1(LIR_qasd, q); }
  LIns *dasq(LIns *q) { return lir_->ins1(LIR_dasq, q); }

  LIns *liveq(LIns *q) { return lir_->ins1(LIR_liveq, q); }
  LIns *livei(LIns *q) { return lir_->ins1(LIR_livei, q); }
  LIns *livef(LIns *q) { return lir_->ins1(LIR_livef, q); }
  LIns *lived(LIns *q) { return lir_->ins1(LIR_lived, q); }

  LIns *comment(const char *s) { return lir_->insComment(s); }

  LIns *call(const char *funcname, LOpcode opcode, AbiKind abi, int argc,
             LIns *args[]);

  /**
  * Completes the fragment, adds a guard record and if all ok, assembles the
  * code.
  * If assembly is successful then the generated code is saved in the parent Jit
  * object
  * by fragment name.
  */
  void *finalize();

  SideExit *createSideExit();
  GuardRecord *createGuardRecord(SideExit *exit);

private:
  // Prohibit copying.
  FunctionBuilderImpl(const FunctionBuilderImpl &) = delete;
  FunctionBuilderImpl &operator=(const FunctionBuilderImpl &) = delete;
};

uint32_t FunctionBuilderImpl::sProfId = 0;

NanoJitContextImpl::NanoJitContextImpl(bool verbose, Config config)
    : verbose_(verbose), config_(config), code_alloc_(&config),
      asm_(code_alloc_, alloc_, alloc_, &logc_, config_) {
  verbose_ = verbose;
  logc_.lcbits = 0;

  lirbuf_ = new (alloc_) LirBuffer(alloc_);
#ifdef DEBUG
  if (verbose) {
    logc_.lcbits = LC_ReadLIR | LC_AfterDCE | LC_Native | LC_RegAlloc |
                   LC_Activation | LC_Bytes;
    lirbuf_->printer = new (alloc_) LInsPrinter(alloc_, LIRASM_NUM_USED_ACCS);
  }
#endif
}

NanoJitContextImpl::~NanoJitContextImpl() {
  Fragments::iterator i;
  for (i = fragments_.begin(); i != fragments_.end(); ++i) {
    delete i->second.fragptr;
  }
}

LirasmFragment *NanoJitContextImpl::get_fragment(const char *name) {
  std::string n(name);
  auto const &result = fragments_.find(n);
  if (result == fragments_.end())
    return nullptr;
  return &result->second;
}

bool NanoJitContextImpl::registerFunction(const std::string &name, void *fptr,
                                          ArgType retval, const ArgType *args,
                                          int argc) {
  for (int i = 0; i < external_functions_.size(); i++) {
    auto &function = external_functions_[i];
    if (function.name == name) {
      return function.callInfo._address == (uintptr_t)fptr;
    }
  }
  if (argc < 0 || argc > MAXARGS) {
    fprintf(
        stderr,
        "Error: cannot register a function that has more than %d arguments\n",
        MAXARGS);
    return false;
  }
  if (!fptr) {
    fprintf(stderr, "Error: cannot register a NULL function\n");
    return false;
  }
  for (int i = 0; i < argc; i++) {
    if (args[i] < ARGTYPE_I && args[i] > ARGTYPE_F) {
      fprintf(stderr, "Error in arg[%d]: Function cannot accept this type of "
                      "argument at present\n",
              i);
      return false;
    }
  }
  if (retval < ARGTYPE_V || retval > ARGTYPE_F) {
    fprintf(stderr, "Error: Function must return a value\n");
    return false;
  }

  uint32_t typeSig = CallInfo::typeSigN(retval, argc, args);
  Function function;
  function.name = name;
  function.callInfo._address = (uintptr_t)fptr;
  function.callInfo._name = "";
  function.callInfo._typesig = typeSig;
  function.callInfo._storeAccSet = ACCSET_STORE_ANY;
  function.callInfo._abi = nanojit::ABI_CDECL; // Assumed to be C calling
                                               // convention, maybe this should
                                               // be a parameter
  function.callInfo._isPure = 0;
  external_functions_.push_back(function);
  return true;
}

int NanoJitContextImpl::lookupFunction(const std::string &name, CallInfo *&ci) {

  const size_t nfuns = external_functions_.size();
  for (size_t i = 0; i < nfuns; i++) {
    if (name == external_functions_[i].name) {
      *ci = external_functions_[i].callInfo;
      return 1;
    }
  }

  Fragments::const_iterator func = fragments_.find(name);
  if (func != fragments_.end()) {
    // The ABI, arg types and ret type will be overridden by the caller.
    if (func->second.mReturnType == RT_DOUBLE) {
      CallInfo target = {
          (uintptr_t)func->second.rdouble, func->second.typeSig, ABI_FASTCALL,
          /*isPure*/ 0, ACCSET_STORE_ANY verbose_only(, func->first.c_str())};
      *ci = target;
    } else if (func->second.mReturnType == RT_FLOAT) {
      CallInfo target = {
          (uintptr_t)func->second.rfloat, func->second.typeSig, ABI_FASTCALL,
          /*isPure*/ 0, ACCSET_STORE_ANY verbose_only(, func->first.c_str())};
      *ci = target;
    } else if (func->second.mReturnType == RT_QUAD) {
      CallInfo target = {
          (uintptr_t)func->second.rquad, func->second.typeSig, ABI_FASTCALL,
          /*isPure*/ 0, ACCSET_STORE_ANY verbose_only(, func->first.c_str())};
      *ci = target;
    } else {
      CallInfo target = {
          (uintptr_t)func->second.rint, func->second.typeSig, ABI_FASTCALL,
          /*isPure*/ 0, ACCSET_STORE_ANY verbose_only(, func->first.c_str())};
      *ci = target;
    }
    return 2;
  } else {
    return 0;
  }
}

FunctionBuilderImpl::FunctionBuilderImpl(NanoJitContextImpl &parent,
                                         const std::string &fragmentName,
                                         ArgType rvalue, const ArgType *args,
                                         int argc, bool optimize)
    : parent_(parent), fragName_(fragmentName), optimize_(optimize),
      bufWriter_(nullptr), cseFilter_(nullptr), exprFilter_(nullptr),
      verboseWriter_(nullptr), validateWriter1_(nullptr),
      validateWriter2_(nullptr), paramCount_(0), rvalue_(rvalue) {
  fragment_ = new Fragment(nullptr verbose_only(
      , (parent_.logc_.lcbits & nanojit::LC_FragProfile) ? sProfId++ : 0));
  fragment_->lirbuf = parent_.lirbuf_;
  parent_.fragments_[fragName_].fragptr = fragment_;

  lir_ = bufWriter_ = new LirBufWriter(parent_.lirbuf_, parent_.config_);
#ifdef DEBUG
  if (optimize) { // don't re-validate if no optimization has taken place
    lir_ = validateWriter2_ = new ValidateWriter(
        lir_, fragment_->lirbuf->printer, "end of writer pipeline");
  }
#endif
#ifdef DEBUG
  if (parent_.verbose_) {
    lir_ = verboseWriter_ = new VerboseWriter(
        parent_.alloc_, lir_, parent_.lirbuf_->printer, &parent_.logc_);
  }
#endif
  if (optimize) {
    lir_ = cseFilter_ = new CseFilter(lir_, LIRASM_NUM_USED_ACCS,
                                      parent_.alloc_, parent_.config_);
  }
  if (optimize) {
    lir_ = exprFilter_ = new ExprFilter(lir_);
  }
#ifdef DEBUG
  lir_ = validateWriter1_ = new ValidateWriter(lir_, fragment_->lirbuf->printer,
                                               "start of writer pipeline");
#endif
  returnTypeBits_ = 0;
  lir_->ins0(LIR_start);
  if (argc < 0)
    argc = 0;
  if (argc > MAXARGS)
    argc = MAXARGS;
  for (int i = 0; i < nanojit::NumSavedRegs; ++i) {
    lir_->insParam(i, 1);
  }
  // For each expected argument
  // we create an instruction
  for (int i = 0; i < argc; i++) {
    args_[i] = args[i];
    params_[i] = insertParameter();
  }
}

FunctionBuilderImpl::~FunctionBuilderImpl() {
  delete validateWriter1_;
  delete validateWriter2_;
  delete verboseWriter_;
  delete exprFilter_;
  delete cseFilter_;
  delete bufWriter_;
}

LIns *FunctionBuilderImpl::getParameter(int pos) {
  if (pos < 0 || pos >= paramCount_)
    return nullptr;
  // TODO not sure if the type conversions
  // below cover all cases - also need
  // tests to validate
  if (args_[pos] == ARGTYPE_I) {
#ifdef NANOJIT_64BIT
    return q2i(params_[pos]);
#else
    return params_[pos];
#endif
  } else {
    return params_[pos];
  }
}

LIns *FunctionBuilderImpl::call(const char *funcname, LOpcode opcode,
                                AbiKind abi, int argc, LIns *argsin[]) {
  if (argc < 0 || argc > MAXARGS)
    return nullptr;

  std::string func(funcname);
  CallInfo *ci = new (parent_.alloc_) CallInfo;

  // We can only call functions previously defined
  // TODO is there a need to handle functions compiled by
  // nanojit differently than externally defined functions.
  // Internals are FASTCALL for example
  int known = parent_.lookupFunction(func, ci);
  if (!known)
    return nullptr;

  ArgType argTypes[MAXARGS]; // In order
  LIns *args[MAXARGS];	// In reverse order
  memset(&args[0], 0, sizeof(args));

  // Nanojit expects parameters in reverse order
  for (int j = argc - 1; j >= 0; j--) {
    NanoAssert(j < MAXARGS); // should give a useful error msg if this fails
    int i = argc - j - 1;
    NanoAssert(i >= 0 && i < argc);
    args[i] = argsin[j];
    if (args[i]->isD())
      argTypes[j] = ARGTYPE_D;
    else if (args[i]->isF())
      argTypes[j] = ARGTYPE_F;
    else if (args[i]->isQ())
      argTypes[j] = ARGTYPE_Q;
    else
      argTypes[j] = ARGTYPE_I;
  }

  // Select return type from opcode.
  ArgType retType = ARGTYPE_P;
  if (opcode == LIR_callv)
    retType = ARGTYPE_V;
  else if (opcode == LIR_calli)
    retType = ARGTYPE_I;
  else if (opcode == LIR_callq)
    retType = ARGTYPE_Q;
  else if (opcode == LIR_calld)
    retType = ARGTYPE_D;
  else if (opcode == LIR_callf)
    retType = ARGTYPE_F;
  else
    return nullptr;

  uint32_t callSiteTypeSig = CallInfo::typeSigN(retType, (int)argc, argTypes);
  if (ci->_typesig != 0 && ci->_typesig != callSiteTypeSig) {
    fprintf(stderr, "Fatal error: mismatch in type signature between callsite "
                    "and function definition\n");
    abort();
  }

  ci->_typesig = callSiteTypeSig;

  return lir_->insCall(ci, args);
}

LIns *FunctionBuilderImpl::reti(LIns *result) {
  NanoAssert(rvalue_ == ARGTYPE_I);
  returnTypeBits_ |= ReturnType::RT_INT;
  return lir_->ins1(LIR_reti, result);
}

LIns *FunctionBuilderImpl::retd(LIns *result) {
  NanoAssert(rvalue_ == ARGTYPE_D);
  returnTypeBits_ |= ReturnType::RT_DOUBLE;
  return lir_->ins1(LIR_retd, result);
}

LIns *FunctionBuilderImpl::retf(LIns *result) {
  NanoAssert(rvalue_ == ARGTYPE_F);
  returnTypeBits_ |= ReturnType::RT_FLOAT;
  return lir_->ins1(LIR_retf, result);
}

LIns *FunctionBuilderImpl::retq(LIns *result) {
  NanoAssert(rvalue_ == ARGTYPE_Q);
  returnTypeBits_ |= ReturnType::RT_QUAD;
  return lir_->ins1(LIR_retq, result);
}

SideExit *FunctionBuilderImpl::createSideExit() {
  SideExit *exit = new (parent_.alloc_) SideExit();
  memset(exit, 0, sizeof(SideExit));
  exit->from = fragment_;
  exit->target = nullptr;
  return exit;
}

GuardRecord *FunctionBuilderImpl::createGuardRecord(SideExit *exit) {
  GuardRecord *rec = new (parent_.alloc_) GuardRecord;
  memset(rec, 0, sizeof(GuardRecord));
  rec->exit = exit;
  exit->addGuard(rec);
  return rec;
}

void *FunctionBuilderImpl::finalize() {
  if (returnTypeBits_ == 0) {
    std::cerr << "warning: no return type in fragment '" << fragName_ << "'"
              << std::endl;

  } else if (returnTypeBits_ != RT_INT && returnTypeBits_ != RT_QUAD &&
             returnTypeBits_ != RT_DOUBLE && returnTypeBits_ != RT_FLOAT) {
    std::cerr << "warning: multiple return types in fragment '" << fragName_
              << "'" << std::endl;
    return nullptr;
  }

  /*
  * Note that it is necessary to mark the parameters as 'live'
  * after the function code is complete - i.e. at the very end. This
  * needed to tell the register allocator to not use the register
  * associated with the parameter.
  */
  for (int i = 0; i < paramCount_; i++) {
    liveq(params_[i]);
  }

  fragment_->lastIns =
      lir_->insGuard(LIR_x, NULL, createGuardRecord(createSideExit()));

  parent_.asm_.compile(fragment_, parent_.alloc_,
                       optimize_ verbose_only(, parent_.lirbuf_->printer));

  if (parent_.asm_.error() != nanojit::None) {
    std::cerr << "error during assembly: ";
    switch (parent_.asm_.error()) {
    case nanojit::BranchTooFar:
      std::cerr << "BranchTooFar";
      break;
    case nanojit::StackFull:
      std::cerr << "StackFull";
      break;
    case nanojit::UnknownBranch:
      std::cerr << "UnknownBranch";
      break;
    case nanojit::None:
      std::cerr << "None";
      break;
    default:
      NanoAssert(0);
      break;
    }
    std::cerr << std::endl;
    std::exit(1);
  }

  LirasmFragment *f;
  f = &parent_.fragments_[fragName_];

  switch (returnTypeBits_) {
  case RT_INT:
    f->rint = (RetInt)((uintptr_t)fragment_->code());
    f->mReturnType = RT_INT;
    f->typeSig = CallInfo::typeSigN(rvalue_, paramCount_, args_);
    return reinterpret_cast<void *>(f->rint);
  case RT_QUAD:
    f->rquad = (RetQuad)((uintptr_t)fragment_->code());
    f->mReturnType = RT_QUAD;
    f->typeSig = CallInfo::typeSigN(rvalue_, paramCount_, args_);
    return reinterpret_cast<void *>(f->rquad);
  case RT_DOUBLE:
    f->rdouble = (RetDouble)((uintptr_t)fragment_->code());
    f->mReturnType = RT_DOUBLE;
    f->typeSig = CallInfo::typeSigN(rvalue_, paramCount_, args_);
    return reinterpret_cast<void *>(f->rdouble);
  case RT_FLOAT:
    f->rfloat = (RetFloat)((uintptr_t)fragment_->code());
    f->mReturnType = RT_FLOAT;
    f->typeSig = CallInfo::typeSigN(rvalue_, paramCount_, args_);
    return reinterpret_cast<void *>(f->rfloat);
  default:
    NanoAssert(0);
    std::cerr << "invalid return type\n";
    break;
  }
  return nullptr;
}
}

using namespace nanojit;

static inline NJXContextRef wrap_context(NanoJitContextImpl *p) {
  return reinterpret_cast<NJXContextRef>(p);
}

static inline NanoJitContextImpl *unwrap_context(NJXContextRef p) {
  return reinterpret_cast<NanoJitContextImpl *>(p);
}

static inline NJXFunctionBuilderRef
wrap_function_builder(FunctionBuilderImpl *p) {
  return reinterpret_cast<NJXFunctionBuilderRef>(p);
}

static inline FunctionBuilderImpl *
unwrap_function_builder(NJXFunctionBuilderRef p) {
  return reinterpret_cast<FunctionBuilderImpl *>(p);
}

static inline NJXLInsRef wrap_ins(LIns *p) {
  return reinterpret_cast<NJXLInsRef>(p);
}

static inline LIns *unwrap_ins(NJXLInsRef p) {
  return reinterpret_cast<LIns *>(p);
}

extern "C" {

NJXContextRef NJX_create_context(int verbose) {
  auto ctx = new NanoJitContextImpl(verbose != 0, Config());
  return wrap_context(ctx);
}

void NJX_destroy_context(NJXContextRef ctx) {
  auto impl = unwrap_context(ctx);
  delete impl;
}

void *NJX_get_function_by_name(NJXContextRef ctx, const char *name) {
  auto impl = unwrap_context(ctx);
  LirasmFragment *f = impl->get_fragment(name);
  if (f) {
    switch (f->mReturnType) {
    case RT_INT:
      return reinterpret_cast<void *>(f->rint);
    case RT_QUAD:
      return reinterpret_cast<void *>(f->rquad);
    case RT_DOUBLE:
      return reinterpret_cast<void *>(f->rdouble);
    case RT_FLOAT:
      return reinterpret_cast<void *>(f->rfloat);
    }
  }
  return nullptr;
}

bool NJX_register_C_function(NJXContextRef context, const char *name,
                             void *fptr, NJXValueKind return_type,
                             const NJXValueKind *args, int argc) {
  auto ctx = unwrap_context(context);
  return ctx->registerFunction(std::string(name), fptr, (ArgType)return_type,
                               (const ArgType *)args, argc);
}

NJXFunctionBuilderRef NJX_create_function_builder(NJXContextRef context,
                                                  const char *name,
                                                  NJXValueKind return_type,
                                                  const NJXValueKind *args,
                                                  int argc, int optimize) {
  if (argc < 0 || argc > NJXMaxArgs) {
    fprintf(stderr,
            "Error: Function cannot accept more than %d arguments at present\n",
            NJXMaxArgs);
    return nullptr;
  }
  for (int i = 0; i < argc; i++) {
    if (args[i] != NJXValueKind_I && args[i] != NJXValueKind_Q) {
      fprintf(stderr, "Error in arg[%d]: Function cannot accept non integer / "
                      "pointer arguments at present\n",
              i);
      return nullptr;
    }
  }
  if (return_type < NJXValueKind_I || return_type > NJXValueKind_F) {
    fprintf(stderr, "Error: Function must return a value\n");
    return nullptr;
  }
  auto impl = new FunctionBuilderImpl(*unwrap_context(context),
                                      std::string(name), (ArgType)return_type,
                                      (ArgType *)args, argc, optimize != 0);
  return wrap_function_builder(impl);
}

void NJX_destroy_function_builder(NJXFunctionBuilderRef fn) {
  auto impl = unwrap_function_builder(fn);
  delete impl;
}

NJXLInsRef NJX_reti(NJXFunctionBuilderRef fn, NJXLInsRef result) {
  return wrap_ins(unwrap_function_builder(fn)->reti(unwrap_ins(result)));
}

NJXLInsRef NJX_retd(NJXFunctionBuilderRef fn, NJXLInsRef result) {
  return wrap_ins(unwrap_function_builder(fn)->retd(unwrap_ins(result)));
}

NJXLInsRef NJX_retf(NJXFunctionBuilderRef fn, NJXLInsRef result) {
  return wrap_ins(unwrap_function_builder(fn)->retf(unwrap_ins(result)));
}

NJXLInsRef NJX_retq(NJXFunctionBuilderRef fn, NJXLInsRef result) {
  return wrap_ins(unwrap_function_builder(fn)->retq(unwrap_ins(result)));
}

NJXLInsRef NJX_ret(NJXFunctionBuilderRef fn) {
  return wrap_ins(unwrap_function_builder(fn)->ret());
}

NJXLInsRef NJX_immi(NJXFunctionBuilderRef fn, int32_t i) {
  return wrap_ins(unwrap_function_builder(fn)->immi(i));
}

NJXLInsRef NJX_immq(NJXFunctionBuilderRef fn, int64_t q) {
  return wrap_ins(unwrap_function_builder(fn)->immq(q));
}

NJXLInsRef NJX_immd(NJXFunctionBuilderRef fn, double d) {
  return wrap_ins(unwrap_function_builder(fn)->immd(d));
}

NJXLInsRef NJX_immf(NJXFunctionBuilderRef fn, float f) {
  return wrap_ins(unwrap_function_builder(fn)->immf(f));
}

/**
* Gets a function parameter.
*/
NJXLInsRef NJX_get_parameter(NJXFunctionBuilderRef fn, int i) {
  return wrap_ins(unwrap_function_builder(fn)->getParameter(i));
}

NJXLInsRef NJX_addi(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->addi(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_addq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->addq(unwrap_ins(lhs), unwrap_ins((rhs))));
}

NJXLInsRef NJX_addd(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->addd(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_addf(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->addf(unwrap_ins(lhs), unwrap_ins((rhs))));
}

NJXLInsRef NJX_subi(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->subi(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_subq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->subq(unwrap_ins(lhs), unwrap_ins((rhs))));
}

NJXLInsRef NJX_subd(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->subd(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_subf(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->subf(unwrap_ins(lhs), unwrap_ins((rhs))));
}

NJXLInsRef NJX_muli(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->muli(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_mulq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->mulq(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_muld(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->muld(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_mulf(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->mulf(unwrap_ins(lhs), unwrap_ins((rhs))));
}

NJXLInsRef NJX_divi(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->divi(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_divq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->divq(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_divd(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->divd(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_divf(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->divf(unwrap_ins(lhs), unwrap_ins((rhs))));
}

NJXLInsRef NJX_modi(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->modi(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_modq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->modq(unwrap_ins(lhs), unwrap_ins((rhs))));
}

NJXLInsRef NJX_negq(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->negq(unwrap_ins(q)));
}
NJXLInsRef NJX_negi(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->negi(unwrap_ins(q)));
}
NJXLInsRef NJX_negf(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->negf(unwrap_ins(q)));
}
NJXLInsRef NJX_negd(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->negd(unwrap_ins(q)));
}

NJXLInsRef NJX_notq(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->notq(unwrap_ins(q)));
}
NJXLInsRef NJX_noti(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->noti(unwrap_ins(q)));
}

NJXLInsRef NJX_andi(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->andi(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_andq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->andq(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_ori(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->ori(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_orq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->orq(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_xori(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->xori(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_xorq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->xorq(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_lshi(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->lshi(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_lshq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->lshq(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_rshi(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->rshi(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_rshq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->rshq(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_rshui(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->rshui(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_rshuq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->rshuq(unwrap_ins(lhs), unwrap_ins((rhs))));
}

NJXLInsRef NJX_eqi(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->eqi(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_eqq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->eqq(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_eqd(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->eqd(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_eqf(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->eqf(unwrap_ins(lhs), unwrap_ins((rhs))));
}

NJXLInsRef NJX_lti(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->lti(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_lei(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->lei(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_ltui(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->ltui(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_leui(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->leui(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_ltq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->ltq(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_leq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->leq(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_ltuq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->ltuq(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_leuq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->leuq(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_ltd(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->ltd(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_led(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->led(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_ltf(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->ltf(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_lef(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->lef(unwrap_ins(lhs), unwrap_ins((rhs))));
}

NJXLInsRef NJX_gti(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->gti(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_gei(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->gei(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_gtui(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->gtui(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_geui(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->geui(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_gtq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->gtq(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_geq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->geq(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_gtuq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->gtuq(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_geuq(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->geuq(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_gtd(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->gtd(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_ged(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->ged(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_gtf(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->gtf(unwrap_ins(lhs), unwrap_ins((rhs))));
}
NJXLInsRef NJX_gef(NJXFunctionBuilderRef fn, NJXLInsRef lhs, NJXLInsRef rhs) {
  return wrap_ins(
      unwrap_function_builder(fn)->gef(unwrap_ins(lhs), unwrap_ins((rhs))));
}

NJXLInsRef NJX_i2q(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->i2q(unwrap_ins(q)));
}
NJXLInsRef NJX_ui2uq(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->ui2uq(unwrap_ins(q)));
}
NJXLInsRef NJX_q2i(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->q2i(unwrap_ins(q)));
}
NJXLInsRef NJX_q2d(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->q2d(unwrap_ins(q)));
}
NJXLInsRef NJX_i2d(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->i2d(unwrap_ins(q)));
}
NJXLInsRef NJX_i2f(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->i2f(unwrap_ins(q)));
}
NJXLInsRef NJX_ui2d(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->ui2d(unwrap_ins(q)));
}
NJXLInsRef NJX_ui2f(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->ui2f(unwrap_ins(q)));
}
NJXLInsRef NJX_f2d(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->f2d(unwrap_ins(q)));
}
NJXLInsRef NJX_d2f(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->d2f(unwrap_ins(q)));
}
NJXLInsRef NJX_d2i(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->d2i(unwrap_ins(q)));
}
NJXLInsRef NJX_f2i(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->f2i(unwrap_ins(q)));
}
NJXLInsRef NJX_d2q(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->d2q(unwrap_ins(q)));
}

NJXLInsRef NJX_liveq(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->liveq(unwrap_ins(q)));
}
NJXLInsRef NJX_livei(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->livei(unwrap_ins(q)));
}
NJXLInsRef NJX_livef(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->livef(unwrap_ins(q)));
}
NJXLInsRef NJX_lived(NJXFunctionBuilderRef fn, NJXLInsRef q) {
  return wrap_ins(unwrap_function_builder(fn)->lived(unwrap_ins(q)));
}

NJXLInsRef NJX_add_label(NJXFunctionBuilderRef fn) {
  return wrap_ins(unwrap_function_builder(fn)->addLabel());
}

NJXLInsRef NJX_alloca(NJXFunctionBuilderRef fn, int32_t size) {
  return wrap_ins(unwrap_function_builder(fn)->allocA(size));
}

NJXLInsRef NJX_br(NJXFunctionBuilderRef fn, NJXLInsRef to) {
  return wrap_ins(unwrap_function_builder(fn)->br(unwrap_ins(to)));
}

NJXLInsRef NJX_cbr_true(NJXFunctionBuilderRef fn, NJXLInsRef cond,
                        NJXLInsRef to) {
  return wrap_ins(
      unwrap_function_builder(fn)->cbrTrue(unwrap_ins(cond), unwrap_ins((to))));
}

NJXLInsRef NJX_cbr_false(NJXFunctionBuilderRef fn, NJXLInsRef cond,
                         NJXLInsRef to) {
  return wrap_ins(unwrap_function_builder(fn)->cbrFalse(unwrap_ins(cond),
                                                        unwrap_ins((to))));
}

NJXLInsRef NJX_choose(NJXFunctionBuilderRef fn, NJXLInsRef cond, NJXLInsRef iftrue,
                   NJXLInsRef iffalse, bool use_cmov) {
  return wrap_ins(unwrap_function_builder(fn)->choose(
      unwrap_ins(cond), unwrap_ins(iftrue), unwrap_ins(iffalse), use_cmov));
}

NJXLInsRef NJX_switch(NJXFunctionBuilderRef fn, NJXLInsRef index,
                      int32_t size) {
  return wrap_ins(
      unwrap_function_builder(fn)->jmpTable(unwrap_ins(index), size));
}

NJXLInsRef NJX_load_c2i(NJXFunctionBuilderRef fn, NJXLInsRef ptr,
                        int32_t offset) {
  return wrap_ins(
      unwrap_function_builder(fn)->loadc2i(unwrap_ins(ptr), offset));
}
NJXLInsRef NJX_load_uc2ui(NJXFunctionBuilderRef fn, NJXLInsRef ptr,
                          int32_t offset) {
  return wrap_ins(
      unwrap_function_builder(fn)->loaduc2ui(unwrap_ins(ptr), offset));
}
NJXLInsRef NJX_load_s2i(NJXFunctionBuilderRef fn, NJXLInsRef ptr,
                        int32_t offset) {
  return wrap_ins(
      unwrap_function_builder(fn)->loads2i(unwrap_ins(ptr), offset));
}
NJXLInsRef NJX_load_us2ui(NJXFunctionBuilderRef fn, NJXLInsRef ptr,
                          int32_t offset) {
  return wrap_ins(
      unwrap_function_builder(fn)->loadus2ui(unwrap_ins(ptr), offset));
}
NJXLInsRef NJX_load_i(NJXFunctionBuilderRef fn, NJXLInsRef ptr,
                      int32_t offset) {
  return wrap_ins(unwrap_function_builder(fn)->loadi(unwrap_ins(ptr), offset));
}
NJXLInsRef NJX_load_q(NJXFunctionBuilderRef fn, NJXLInsRef ptr,
                      int32_t offset) {
  return wrap_ins(unwrap_function_builder(fn)->loadq(unwrap_ins(ptr), offset));
}
NJXLInsRef NJX_load_f(NJXFunctionBuilderRef fn, NJXLInsRef ptr,
                      int32_t offset) {
  return wrap_ins(unwrap_function_builder(fn)->loadf(unwrap_ins(ptr), offset));
}
NJXLInsRef NJX_load_d(NJXFunctionBuilderRef fn, NJXLInsRef ptr,
                      int32_t offset) {
  return wrap_ins(unwrap_function_builder(fn)->loadd(unwrap_ins(ptr), offset));
}
NJXLInsRef NJX_load_f2d(NJXFunctionBuilderRef fn, NJXLInsRef ptr,
                        int32_t offset) {
  return wrap_ins(
      unwrap_function_builder(fn)->loadf2d(unwrap_ins(ptr), offset));
}

NJXLInsRef NJX_store_i2c(NJXFunctionBuilderRef fn, NJXLInsRef value,
                         NJXLInsRef ptr, int32_t offset) {
  return wrap_ins(unwrap_function_builder(fn)->storei2c(
      unwrap_ins(value), unwrap_ins(ptr), offset));
}
NJXLInsRef NJX_store_i2s(NJXFunctionBuilderRef fn, NJXLInsRef value,
                         NJXLInsRef ptr, int32_t offset) {
  return wrap_ins(unwrap_function_builder(fn)->storei2s(
      unwrap_ins(value), unwrap_ins(ptr), offset));
}
NJXLInsRef NJX_store_i(NJXFunctionBuilderRef fn, NJXLInsRef value,
                       NJXLInsRef ptr, int32_t offset) {
  return wrap_ins(unwrap_function_builder(fn)->storei(unwrap_ins(value),
                                                      unwrap_ins(ptr), offset));
}
NJXLInsRef NJX_store_q(NJXFunctionBuilderRef fn, NJXLInsRef value,
                       NJXLInsRef ptr, int32_t offset) {
  return wrap_ins(unwrap_function_builder(fn)->storeq(unwrap_ins(value),
                                                      unwrap_ins(ptr), offset));
}
NJXLInsRef NJX_store_d(NJXFunctionBuilderRef fn, NJXLInsRef value,
                       NJXLInsRef ptr, int32_t offset) {
  return wrap_ins(unwrap_function_builder(fn)->stored(unwrap_ins(value),
                                                      unwrap_ins(ptr), offset));
}
NJXLInsRef NJX_store_f(NJXFunctionBuilderRef fn, NJXLInsRef value,
                       NJXLInsRef ptr, int32_t offset) {
  return wrap_ins(unwrap_function_builder(fn)->storef(unwrap_ins(value),
                                                      unwrap_ins(ptr), offset));
}

bool NJX_is_i(NJXLInsRef ins) { return unwrap_ins(ins)->isI(); }
bool NJX_is_q(NJXLInsRef ins) { return unwrap_ins(ins)->isQ(); }
bool NJX_is_d(NJXLInsRef ins) { return unwrap_ins(ins)->isD(); }
bool NJX_is_f(NJXLInsRef ins) { return unwrap_ins(ins)->isF(); }

/**
* Sets the target of a jump instruction
*/
void NJX_set_jmp_target(NJXLInsRef jmp, NJXLInsRef target) {
  auto jmpins = unwrap_ins(jmp);
  auto targetins = unwrap_ins(target);
  jmpins->setTarget(targetins);
}

void NJX_set_switch_target(NJXLInsRef switchins, uint32_t index,
                           NJXLInsRef target) {
  auto jmpins = unwrap_ins(switchins);
  auto targetins = unwrap_ins(target);
  jmpins->setTarget(index, targetins);
}

static NJXLInsRef NJX_call(NJXFunctionBuilderRef fn, const char *funcname,
                           LOpcode opcode, NJXCallAbiKind abi, int nargs,
                           NJXLInsRef args[]) {
  if (nargs > MAXARGS) {
    fprintf(stderr, "Only upto %d arguments allowed in a call\n", MAXARGS);
    return nullptr;
  }
  auto builder = unwrap_function_builder(fn);
  AbiKind abikind;
  switch (abi) {
  case NJXCallAbiKind::NJX_CALLABI_CDECL:
    abikind = AbiKind::ABI_CDECL;
    break;
  case NJXCallAbiKind::NJX_CALLABI_FASTCALL:
    abikind = AbiKind::ABI_FASTCALL;
    break;
  case NJXCallAbiKind::NJX_CALLABI_STDCALL:
    abikind = AbiKind::ABI_STDCALL;
    break;
  case NJXCallAbiKind::NJX_CALLABI_THISCALL:
    abikind = AbiKind::ABI_THISCALL;
    break;
  default:
    return nullptr;
  }
  LIns *arguments[MAXARGS];
  for (int i = 0; i < nargs; i++) {
    arguments[i] = unwrap_ins(args[i]);
  }
  return wrap_ins(builder->call(funcname, opcode, abikind, nargs, arguments));
}

NJXLInsRef NJX_callv(NJXFunctionBuilderRef fn, const char *funcname,
                     NJXCallAbiKind abi, int nargs, NJXLInsRef args[]) {
  return NJX_call(fn, funcname, LIR_callv, abi, nargs, args);
}
NJXLInsRef NJX_calli(NJXFunctionBuilderRef fn, const char *funcname,
                     NJXCallAbiKind abi, int nargs, NJXLInsRef args[]) {
  return NJX_call(fn, funcname, LIR_calli, abi, nargs, args);
}
NJXLInsRef NJX_callq(NJXFunctionBuilderRef fn, const char *funcname,
                     NJXCallAbiKind abi, int nargs, NJXLInsRef args[]) {
  return NJX_call(fn, funcname, LIR_callq, abi, nargs, args);
}
NJXLInsRef NJX_callf(NJXFunctionBuilderRef fn, const char *funcname,
                     NJXCallAbiKind abi, int nargs, NJXLInsRef args[]) {
  return NJX_call(fn, funcname, LIR_callf, abi, nargs, args);
}
NJXLInsRef NJX_calld(NJXFunctionBuilderRef fn, const char *funcname,
                     NJXCallAbiKind abi, int nargs, NJXLInsRef args[]) {
  return NJX_call(fn, funcname, LIR_calld, abi, nargs, args);
}

NJXLInsRef NJX_comment(NJXFunctionBuilderRef fn, const char *s) {
  return wrap_ins(unwrap_function_builder(fn)->comment(s));
}

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
void *NJX_finalize(NJXFunctionBuilderRef fn) {
  return unwrap_function_builder(fn)->finalize();
}
}

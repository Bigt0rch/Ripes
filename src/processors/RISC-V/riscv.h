#pragma once

#include <functional>

#include "rv_instrparser.h"

#include "isa/rvisainfo_common.h"

namespace Ripes {

namespace RVISA {

template <unsigned XLEN>
ProcessorISAInfo supportsISA() {
  using RVISAInfo = ISAInfo<XLenToRVISA<XLEN>()>;
  return ProcessorISAInfo{std::make_shared<RVISAInfo>(QStringList()),
                          RVISAInfo::getSupportedExtensions(),
                          RVISAInfo::getDefaultExtensions()};
}

template <unsigned XLEN>
std::shared_ptr<const ISAInfoBase> fullISA() {
  return ISAInfoRegistry::getSupportedISA<XLenToRVISA<XLEN>()>();
}

} // namespace RVISA

constexpr int c_RVInstrWidth = 32; // Width of instructions
constexpr int c_RVRegs = 32;       // Number of registers
constexpr int c_RVRegsBits =
    vsrtl::ceillog2(c_RVRegs); // Width of operand to index into registers

/** Instruction set enumerations */
enum class RVInstrType { R, I, S, B, U, J };

enum class RVInstr {
  NOP,
  /* RV32I Base Instruction Set */
  LUI,
  AUIPC,
  JAL,
  JALR,
  BEQ,
  BNE,
  BLT,
  BGE,
  BLTU,
  BGEU,
  LB,
  LH,
  LW,
  LBU,
  LHU,
  SB,
  SH,
  SW,
  ADDI,
  SLTI,
  SLTIU,
  XORI,
  ORI,
  ANDI,
  SLLI,
  SRLI,
  SRAI,
  ADD,
  SUB,
  SLL,
  SLT,
  SLTU,
  XOR,
  SRL,
  SRA,
  OR,
  AND,
  ECALL,

  /* RV32M Standard Extension */
  MUL,
  MULH,
  MULHSU,
  MULHU,
  DIV,
  DIVU,
  REM,
  REMU,

  /* RV64I Base Instruction Set */
  ADDIW,
  SLLIW,
  SRLIW,
  SRAIW,
  ADDW,
  SUBW,
  SLLW,
  SRLW,
  SRAW,
  LWU,
  LD,
  SD,

  /* RV64M Standard Extension */
  MULW,
  DIVW,
  DIVUW,
  REMW,
  REMUW,

  /* ZiCSR */
  MRET,
  CSRRW,
  CSRRS,
  CSRRC,
  CSRRWI,
  CSRRSI,
  CSRRCI,

  /* Unknown OP */
  UNK
};

enum class CSR {
  MSTATUS,
  MTVEC,
  MIE,
  MEPC,
  MIP,
  MCAUSE,
  MTVAL
};

/** Datapath enumerations */
enum class ALUOp {
  NOP,
  ADD,
  SUB,
  MUL,
  DIV,
  AND,
  OR,
  XOR,
  SL,
  SRA,
  SRL,
  LUI,
  LT,
  LTU,
  EQ,
  MULH,
  MULHU,
  MULHSU,
  DIVU,
  REM,
  REMU,
  SLW,
  SRLW,
  SRAW,
  ADDW,
  SUBW,
  MULW,
  DIVW,
  DIVUW,
  REMW,
  REMUW
};

enum class CSRWrSrc { IMM, REG };
enum class RegWrTrapSrc { MEMREAD, ALURES, PC4, CSR };
enum class RegWrSrc { MEMREAD, ALURES, PC4 };
enum class AluSrc1 { REG1, PC };
enum class AluSrc2 { REG2, IMM };
enum class CompOp { NOP, EQ, NE, LT, LTU, GE, GEU };
enum class MemOp { NOP, LB, LH, LW, LBU, LHU, SB, SH, SW, LWU, LD, SD };
enum ECALL { none, print_int = 1, print_char = 2, print_string = 4, exit = 10 };
enum PcSrc { PC4 = 0, ALU = 1 };
enum PcSrc2 { PC = 0, EPC = 1 };
enum PcSrc3 { E_PC = 0, RTI = 1 };
enum MepcSrc { MEPC_WDATA = 0, MEPC_PC = 1 };
enum McauseSrc { MCAUSE_WDATA = 0, MCAUSE_CAUSE = 1 };
enum PcInc { INC2 = 0, INC4 = 1 };

/** Instruction field parser */
class RVInstrParser {
public:
  static RVInstrParser *getParser() {
    static RVInstrParser parser;
    return &parser;
  }

  std::vector<uint32_t> decodeU32Instr(const uint32_t &instr) const {
    return m_decodeU32Instr(instr);
  }
  std::vector<uint32_t> decodeJ32Instr(const uint32_t &instr) const {
    return m_decodeJ32Instr(instr);
  }
  std::vector<uint32_t> decodeI32Instr(const uint32_t &instr) const {
    return m_decodeI32Instr(instr);
  }
  std::vector<uint32_t> decodeS32Instr(const uint32_t &instr) const {
    return m_decodeS32Instr(instr);
  }
  std::vector<uint32_t> decodeR32Instr(const uint32_t &instr) const {
    return m_decodeR32Instr(instr);
  }
  std::vector<uint32_t> decodeB32Instr(const uint32_t &instr) const {
    return m_decodeB32Instr(instr);
  }

  // RVC
  std::vector<uint32_t> decodeCA16Instr(const uint32_t &instr) const {
    return m_decodeCA16Instr(instr);
  }
  std::vector<uint32_t> decodeCI16Instr(const uint32_t &instr) const {
    return m_decodeCI16Instr(instr);
  }
  std::vector<uint32_t> decodeCS16Instr(const uint32_t &instr) const {
    return m_decodeCS16Instr(instr);
  }
  std::vector<uint32_t> decodeCIW16Instr(const uint32_t &instr) const {
    return m_decodeCIW16Instr(instr);
  }
  std::vector<uint32_t> decodeCSS16Instr(const uint32_t &instr) const {
    return m_decodeCSS16Instr(instr);
  }
  std::vector<uint32_t> decodeCJ16Instr(const uint32_t &instr) const {
    return m_decodeCJ16Instr(instr);
  }
  std::vector<uint32_t> decodeCB16Instr(const uint32_t &instr) const {
    return m_decodeCB16Instr(instr);
  }
  std::vector<uint32_t> decodeCB216Instr(const uint32_t &instr) const {
    return m_decodeCB216Instr(instr);
  }

private:
  RVInstrParser() {
    m_decodeR32Instr = generateInstrParser<uint32_t>(
        std::vector<int>{7, 5, 3, 5, 5, 7}); // from LSB to MSB
    m_decodeI32Instr =
        generateInstrParser<uint32_t>(std::vector<int>{7, 5, 3, 5, 12});
    m_decodeS32Instr =
        generateInstrParser<uint32_t>(std::vector<int>{7, 5, 3, 5, 5, 7});
    m_decodeB32Instr =
        generateInstrParser<uint32_t>(std::vector<int>{7, 1, 4, 3, 5, 5, 6, 1});
    m_decodeU32Instr =
        generateInstrParser<uint32_t>(std::vector<int>{7, 5, 20});
    m_decodeJ32Instr =
        generateInstrParser<uint32_t>(std::vector<int>{7, 5, 8, 1, 10, 1});

    // RVC
    m_decodeCA16Instr = generateInstrParser<uint32_t>(
        std::vector<int>{2, 3, 2, 3, 2, 1, 3, 16});
    m_decodeCI16Instr =
        generateInstrParser<uint32_t>(std::vector<int>{2, 5, 5, 1, 3, 16});
    m_decodeCS16Instr =
        generateInstrParser<uint32_t>(std::vector<int>{2, 3, 2, 3, 3, 3, 16});
    m_decodeCIW16Instr =
        generateInstrParser<uint32_t>(std::vector<int>{2, 3, 8, 3, 16});
    m_decodeCSS16Instr =
        generateInstrParser<uint32_t>(std::vector<int>{2, 5, 6, 3, 16});
    m_decodeCJ16Instr =
        generateInstrParser<uint32_t>(std::vector<int>{2, 11, 3, 16});
    m_decodeCB16Instr =
        generateInstrParser<uint32_t>(std::vector<int>{2, 5, 3, 3, 3, 16});
    m_decodeCB216Instr =
        generateInstrParser<uint32_t>(std::vector<int>{2, 5, 3, 2, 1, 3, 16});
  }
  decode_functor<uint32_t> m_decodeU32Instr;
  decode_functor<uint32_t> m_decodeJ32Instr;
  decode_functor<uint32_t> m_decodeI32Instr;
  decode_functor<uint32_t> m_decodeS32Instr;
  decode_functor<uint32_t> m_decodeR32Instr;
  decode_functor<uint32_t> m_decodeB32Instr;

  // RVC
  decode_functor<uint32_t> m_decodeCA16Instr;
  decode_functor<uint32_t> m_decodeCI16Instr;
  decode_functor<uint32_t> m_decodeCS16Instr;
  decode_functor<uint32_t> m_decodeCIW16Instr;
  decode_functor<uint32_t> m_decodeCSS16Instr;
  decode_functor<uint32_t> m_decodeCJ16Instr;
  decode_functor<uint32_t> m_decodeCB16Instr;
  decode_functor<uint32_t> m_decodeCB216Instr;
};

} // namespace Ripes

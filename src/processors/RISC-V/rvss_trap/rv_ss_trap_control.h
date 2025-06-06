#pragma once

#include "../rv_control.h"
#include "../riscv.h"

namespace vsrtl {
namespace core {

class TrapControl : public Component {
public:
  /* clang-format off */
    static CompOp do_comp_ctrl(RVInstr opc) {
        switch(opc){
            case RVInstr::BEQ: return CompOp::EQ;
            case RVInstr::BNE: return CompOp::NE;
            case RVInstr::BLT: return CompOp::LT;
            case RVInstr::BGE: return CompOp::GE;
            case RVInstr::BLTU: return CompOp::LTU;
            case RVInstr::BGEU: return CompOp::GEU;
            default: return CompOp::NOP;
        }
    }

    static VSRTL_VT_U do_branch_ctrl(RVInstr opc) {
        switch(opc){
            case RVInstr::BEQ: case RVInstr::BNE: case RVInstr::BLT:
            case RVInstr::BGE: case RVInstr::BLTU: case RVInstr::BGEU:
                return 1;
            default:
                return 0;
        }
    }

    static VSRTL_VT_U do_jump_ctrl(RVInstr opc) {
        switch(opc){
            case RVInstr::JAL: case RVInstr::JALR:
                return 1;
            default:
                return 0;
        }
    }

    static MemOp do_mem_ctrl(RVInstr opc) {
        switch(opc){
            case RVInstr::SB: return MemOp::SB;
            case RVInstr::SH: return MemOp::SH;
            case RVInstr::SW: return MemOp::SW;
            case RVInstr::SD: return MemOp::SD;
            case RVInstr::LB: return MemOp::LB;
            case RVInstr::LH: return MemOp::LH;
            case RVInstr::LW: return MemOp::LW;
            case RVInstr::LD: return MemOp::LD;
            case RVInstr::LBU: return MemOp::LBU;
            case RVInstr::LHU: return MemOp::LHU;
            case RVInstr::LWU: return MemOp::LWU;
            default:
                return MemOp::NOP;
        }
    }

  static VSRTL_VT_U do_reg_do_write_ctrl(RVInstr opc) {
      switch(opc) {
      case RVInstr::LUI:
      case RVInstr::AUIPC:

        // Arithmetic-immediate instructions
      case RVInstr::ADDI: case RVInstr::SLTI: case RVInstr::SLTIU: case RVInstr::XORI:
      case RVInstr::ORI: case RVInstr::ANDI: case RVInstr::SLLI: case RVInstr::SRLI:
      case RVInstr::SRAI: case RVInstr::ADDIW: case RVInstr::SLLIW: case RVInstr::SRLIW:
      case RVInstr::SRAIW:

        // Arithmetic instructions
      case RVInstr::MUL: case RVInstr::MULH: case RVInstr:: MULHSU: case RVInstr::MULHU:
      case RVInstr::DIV: case RVInstr::DIVU: case RVInstr::REM: case RVInstr::REMU:
      case RVInstr::ADD: case RVInstr::SUB: case RVInstr::SLL: case RVInstr::SLT:
      case RVInstr::SLTU: case RVInstr::XOR: case RVInstr::SRL: case RVInstr::SRA:
      case RVInstr::OR: case RVInstr::AND: case RVInstr::ADDW: case RVInstr::SUBW:
      case RVInstr::SLLW: case RVInstr::SRLW: case RVInstr::SRAW: case RVInstr::MULW:
      case RVInstr::DIVW: case RVInstr::DIVUW: case RVInstr::REMW: case RVInstr::REMUW:

        // Load instructions
      case RVInstr::LB: case RVInstr::LH: case RVInstr::LW: case RVInstr::LBU: case RVInstr::LHU:
      case RVInstr::LWU: case RVInstr::LD:

        // Jump instructions
      case RVInstr::JALR:
      case RVInstr::JAL:

        //ZiCSR Instructions
      case RVInstr::CSRRC: case RVInstr::CSRRCI: case RVInstr::CSRRS:
      case RVInstr::CSRRSI: case RVInstr::CSRRW: case RVInstr::CSRRWI:
        return 1;
      default: return 0;
      }
    }

  static RegWrTrapSrc do_reg_wr_src_ctrl(RVInstr opc) {
      switch(opc){
        // Load instructions
      case RVInstr::LB: case RVInstr::LH: case RVInstr::LW: case RVInstr::LBU:
      case RVInstr::LHU: case RVInstr::LWU: case RVInstr::LD:
        return RegWrTrapSrc::MEMREAD;

        // Jump instructions
      case RVInstr::JALR:
      case RVInstr::JAL:
        return RegWrTrapSrc::PC4;

        //CSR instructions
      case RVInstr::CSRRC: case RVInstr::CSRRS: case RVInstr::CSRRCI:
      case RVInstr::CSRRSI: case RVInstr::CSRRW: case RVInstr::CSRRWI:
        return RegWrTrapSrc::CSR;

      default:
        return RegWrTrapSrc::ALURES;
      }
    }

    static AluSrc1 do_alu_op1_ctrl(RVInstr opc) {
        switch(opc) {
            case RVInstr::AUIPC: case RVInstr::JAL:
            case RVInstr::BEQ: case RVInstr::BNE: case RVInstr::BLT:
            case RVInstr::BGE: case RVInstr::BLTU: case RVInstr::BGEU:
                return AluSrc1::PC;
            default:
                return AluSrc1::REG1;
        }
    }

    static AluSrc2 do_alu_op2_ctrl(RVInstr opc) {
        switch(opc) {
        case RVInstr::LUI:
        case RVInstr::AUIPC:
            return AluSrc2::IMM;

        // Arithmetic-immediate instructions
        case RVInstr::ADDI: case RVInstr::SLTI: case RVInstr::SLTIU: case RVInstr::XORI:
        case RVInstr::ORI: case RVInstr::ANDI: case RVInstr::SLLI: case RVInstr::SRLI:
        case RVInstr::SRAI: case RVInstr::ADDIW: case RVInstr::SLLIW: case RVInstr::SRLIW:
        case RVInstr::SRAIW:
            return AluSrc2::IMM;

        // Arithmetic instructions
        case RVInstr::MUL: case RVInstr::MULH: case RVInstr:: MULHSU: case RVInstr::MULHU:
        case RVInstr::DIV: case RVInstr::DIVU: case RVInstr::REM: case RVInstr::REMU:
        case RVInstr::ADD: case RVInstr::SUB: case RVInstr::SLL: case RVInstr::SLT:
        case RVInstr::SLTU: case RVInstr::XOR: case RVInstr::SRL: case RVInstr::SRA:
        case RVInstr::OR: case RVInstr::AND: case RVInstr::ADDW: case RVInstr::SUBW:
        case RVInstr::SLLW: case RVInstr::SRLW: case RVInstr::SRAW: case RVInstr::MULW:
        case RVInstr::DIVW: case RVInstr::DIVUW: case RVInstr::REMW: case RVInstr::REMUW:
            return AluSrc2::REG2;

        // Load/Store instructions
        case RVInstr::LB: case RVInstr::LH: case RVInstr::LW: case RVInstr::LBU: case RVInstr::LHU:
        case RVInstr::SB: case RVInstr::SH: case RVInstr::SW: case RVInstr::LWU: case RVInstr::LD:
        case RVInstr::SD:
            return AluSrc2::IMM;

        // Branch instructions
        case RVInstr::BEQ: case RVInstr::BNE: case RVInstr::BLT:
        case RVInstr::BGE: case RVInstr::BLTU: case RVInstr::BGEU:
            return AluSrc2::IMM;

        // Jump instructions
        case RVInstr::JALR:
        case RVInstr::JAL:
            return AluSrc2::IMM;

        default:
            return AluSrc2::REG2;
        }
    }

    static ALUOp do_alu_ctrl(RVInstr opc) {
        switch(opc) {
            case RVInstr::LB: case RVInstr::LH: case RVInstr::LW: case RVInstr::LBU: case RVInstr::LHU:
            case RVInstr::SB: case RVInstr::SH: case RVInstr::SW: case RVInstr::LWU: case RVInstr::LD:
            case RVInstr::SD:
                return ALUOp::ADD;
            case RVInstr::LUI:
                return ALUOp::LUI;
            case RVInstr::JAL: case RVInstr::JALR: case RVInstr::AUIPC:
            case RVInstr::ADD: case RVInstr::ADDI:
            case RVInstr::BEQ: case RVInstr::BNE: case RVInstr::BLT:
            case RVInstr::BGE: case RVInstr::BLTU: case RVInstr::BGEU:
                return ALUOp::ADD;
            case RVInstr::SUB: return ALUOp::SUB;
            case RVInstr::SLT: case RVInstr::SLTI:
                return ALUOp::LT;
            case RVInstr::SLTU: case RVInstr::SLTIU:
                return ALUOp::LTU;
            case RVInstr::XOR: case RVInstr::XORI:
                return ALUOp::XOR;
            case RVInstr::OR: case RVInstr::ORI:
                return ALUOp::OR;
            case RVInstr::AND: case RVInstr::ANDI:
                return ALUOp::AND;
            case RVInstr::SLL: case RVInstr::SLLI:
                return ALUOp::SL;
            case RVInstr::SRL: case RVInstr::SRLI:
                return ALUOp::SRL;
            case RVInstr::SRA: case RVInstr::SRAI:
                return ALUOp::SRA;
            case RVInstr::MUL   : return ALUOp::MUL;
            case RVInstr::MULH  : return ALUOp::MULH;
            case RVInstr::MULHU : return ALUOp::MULHU;
            case RVInstr::MULHSU: return ALUOp::MULHSU;
            case RVInstr::DIV   : return ALUOp::DIV;
            case RVInstr::DIVU  : return ALUOp::DIVU;
            case RVInstr::REM   : return ALUOp::REM;
            case RVInstr::REMU  : return ALUOp::REMU;
            case RVInstr::ADDIW : return ALUOp::ADDW;
            case RVInstr::SLLIW : return ALUOp::SLW;
            case RVInstr::SRLIW : return ALUOp::SRLW;
            case RVInstr::SRAIW : return ALUOp::SRAW;
            case RVInstr::ADDW  : return ALUOp::ADDW ;
            case RVInstr::SUBW  : return ALUOp::SUBW ;
            case RVInstr::SLLW  : return ALUOp::SLW ;
            case RVInstr::SRLW  : return ALUOp::SRLW ;
            case RVInstr::SRAW  : return ALUOp::SRAW ;
            case RVInstr::MULW  : return ALUOp::MULW ;
            case RVInstr::DIVW  : return ALUOp::DIVW ;
            case RVInstr::DIVUW : return ALUOp::DIVUW;
            case RVInstr::REMW  : return ALUOp::REMW ;
            case RVInstr::REMUW : return ALUOp::REMUW;

            default: return ALUOp::NOP;
        }
    }

    static VSRTL_VT_U do_do_mem_write_ctrl(RVInstr opc) {
        switch(opc) {
            case RVInstr::SB: case RVInstr::SH: case RVInstr::SW: case RVInstr::SD:
                return 1;
            default: return 0;
        }
    }

    static VSRTL_VT_U do_do_read_ctrl(RVInstr opc) {
        switch(opc) {
            case RVInstr::LB: case RVInstr::LH: case RVInstr::LW: case RVInstr::LBU:
            case RVInstr::LHU: case RVInstr::LWU: case RVInstr::LD:
                return 1;
            default: return 0;
        }
    }
  TrapControl(const std::string &name, SimComponent *parent)
      : Component(name, parent) {

      comp_ctrl << [=] { return do_comp_ctrl(opcode.eValue<RVInstr>()); };
      do_branch << [=] { return do_branch_ctrl(opcode.eValue<RVInstr>()); };
      do_jump << [=] { return do_jump_ctrl(opcode.eValue<RVInstr>()); };
      mem_ctrl << [=] { return do_mem_ctrl(opcode.eValue<RVInstr>()); };
      reg_do_write_ctrl <<
          [=] { return do_reg_do_write_ctrl(opcode.eValue<RVInstr>()); };
      reg_wr_src_ctrl <<
          [=] { return do_reg_wr_src_ctrl(opcode.eValue<RVInstr>()); };
      alu_op1_ctrl << [=] { return do_alu_op1_ctrl(opcode.eValue<RVInstr>()); };
      alu_op2_ctrl << [=] { return do_alu_op2_ctrl(opcode.eValue<RVInstr>()); };
      alu_ctrl << [=] { return do_alu_ctrl(opcode.eValue<RVInstr>()); };
      mem_do_write_ctrl <<
          [=] { return do_do_mem_write_ctrl(opcode.eValue<RVInstr>()); };
      mem_do_read_ctrl <<
          [=] { return do_do_read_ctrl(opcode.eValue<RVInstr>()); };

    // Enables for each CSR
    csr_mtvec_en << [=] {
      switch(opcode.eValue<RVInstr>()) {
        case RVInstr::CSRRW: case RVInstr::CSRRWI:
        case RVInstr::CSRRS: case RVInstr::CSRRSI:
          return isCsrWrite(CSR::MTVEC);
        default: return (VSRTL_VT_U)0;
      }
    };
    csr_mtval_en << [=] {
      switch(opcode.eValue<RVInstr>()) {
      case RVInstr::CSRRW: case RVInstr::CSRRWI:
      case RVInstr::CSRRS: case RVInstr::CSRRSI:
        return isCsrWrite(CSR::MTVAL);
      default: return (VSRTL_VT_U)0;
      }
    };
    csr_mstatus_en << [=] {
      switch(opcode.eValue<RVInstr>()) {
      case RVInstr::CSRRW: case RVInstr::CSRRWI:
      case RVInstr::CSRRS: case RVInstr::CSRRSI:
        return isCsrWrite(CSR::MSTATUS);
      default: return (VSRTL_VT_U)0;
      }
    };
    csr_mepc_en << [=] {
      switch(opcode.eValue<RVInstr>()) {
        case RVInstr::CSRRW: case RVInstr::CSRRWI:
        case RVInstr::CSRRS: case RVInstr::CSRRSI:
          return isCsrWrite(CSR::MEPC);
        default: return (VSRTL_VT_U)0;
      }
    };
    csr_mip_en << [=] {
      switch(opcode.eValue<RVInstr>()) {
        case RVInstr::CSRRW: case RVInstr::CSRRWI:
        case RVInstr::CSRRS: case RVInstr::CSRRSI:
          return isCsrWrite(CSR::MIP);
        default: return (VSRTL_VT_U)0;
      }
    };
    csr_mie_en << [=] {
      switch(opcode.eValue<RVInstr>()) {
        case RVInstr::CSRRW: case RVInstr::CSRRWI:
        case RVInstr::CSRRS: case RVInstr::CSRRSI:
          return isCsrWrite(CSR::MIE);
        default: return (VSRTL_VT_U)0;
      }
    };
    csr_mcause_en << [=] {
      switch(opcode.eValue<RVInstr>()) {
        case RVInstr::CSRRW: case RVInstr::CSRRWI:
        case RVInstr::CSRRS: case RVInstr::CSRRSI:
          return isCsrWrite(CSR::MCAUSE);
        default: return (VSRTL_VT_U)0;
      }
    };

    csr_op << [=] {
      switch(opcode.eValue<RVInstr>()) {
      case RVInstr::CSRRW: case RVInstr::CSRRWI:
        return (VSRTL_VT_U)0;
      case RVInstr::CSRRC: case RVInstr::CSRRCI:
        return (VSRTL_VT_U)1;
      case RVInstr::CSRRS: case RVInstr::CSRRSI:
        return (VSRTL_VT_U)2;

      default:
        return (VSRTL_VT_U)0;
      }
    };

    // MRET
    isMret << [=] { return static_cast<VSRTL_VT_U>(opcode.uValue() == static_cast<unsigned>(RVInstr::MRET)); };

    csr_wr_select << [=] {
      switch(opcode.eValue<RVInstr>()) {
        case RVInstr::CSRRC: case RVInstr::CSRRS:
        case RVInstr::CSRRW:
          return CSRWrSrc::REG;
        case RVInstr::CSRRCI: case RVInstr::CSRRSI:
        case RVInstr::CSRRWI:
        return CSRWrSrc::IMM;
        default: return CSRWrSrc::IMM;
      }
    };
  }


  VSRTL_VT_U isCsrWrite(CSR csr) {
    return csr_idx.uValue() == static_cast<unsigned>(csr);
  }





  INPUTPORT_ENUM(opcode, RVInstr);

  OUTPUTPORT(reg_do_write_ctrl, 1);
  OUTPUTPORT(mem_do_write_ctrl, 1);
  OUTPUTPORT(mem_do_read_ctrl, 1);
  OUTPUTPORT(do_branch, 1);
  OUTPUTPORT(do_jump, 1);
  OUTPUTPORT_ENUM(comp_ctrl, CompOp);
  OUTPUTPORT_ENUM(reg_wr_src_ctrl, RegWrTrapSrc);
  OUTPUTPORT_ENUM(mem_ctrl, MemOp);
  OUTPUTPORT_ENUM(alu_op1_ctrl, AluSrc1);
  OUTPUTPORT_ENUM(alu_op2_ctrl, AluSrc2);
  OUTPUTPORT_ENUM(alu_ctrl, ALUOp);

  // New Inputs
  INPUTPORT_ENUM(csr_idx, CSR);

  // New Outputs
  OUTPUTPORT(csr_mtvec_en, 1);
  OUTPUTPORT(csr_mtval_en, 1);
  OUTPUTPORT(csr_mstatus_en, 1);
  OUTPUTPORT(csr_mip_en, 1);
  OUTPUTPORT(csr_mie_en, 1);
  OUTPUTPORT(csr_mcause_en, 1);
  OUTPUTPORT(csr_mepc_en, 1);

  OUTPUTPORT(csr_op, 2);

  OUTPUTPORT_ENUM(csr_wr_select, CSRWrSrc);

  OUTPUTPORT(isMret, 1);
};

} // namespace core
} // namespace vsrtl


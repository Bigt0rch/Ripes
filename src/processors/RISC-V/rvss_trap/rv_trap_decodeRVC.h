#pragma once

#include "../riscv.h"
#include "rv_trap_decode.h"
#include "../rv_uncompress.h"
#include "VSRTL/core/vsrtl_component.h"

namespace vsrtl {
namespace core {
using namespace Ripes;

template <unsigned XLEN>
class UNKChecker : public Component {
public:
  UNKChecker(std::string name, SimComponent *parent) : Component(name, parent) {
    out << [=] { return static_cast<RVInstr>(opcode.uValue()) == RVInstr::UNK ? 1 : 0; };
  }
  INPUTPORT_ENUM(opcode, RVInstr);
  OUTPUTPORT(out, 1);
};

template <unsigned XLEN>
class DecodeRVC_Trap : public Component {
public:
  void setISA(const std::shared_ptr<ISAInfoBase> &isa) {
    decode->setISA(isa);
    uncompress->setISA(isa);
  }
  DecodeRVC_Trap(std::string name, SimComponent *parent) : Component(name, parent) {
    instr >> uncompress->instr;

    uncompress->Pc_Inc >> Pc_Inc;
    uncompress->exp_instr >> decode->instr;
    uncompress->exp_instr >> exp_instr;

    decode->opcode >> opcode;
    decode->opcode >> unk_checker->opcode;
    decode->wr_reg_idx >> wr_reg_idx;
    decode->r1_reg_idx >> r1_reg_idx;
    decode->r2_reg_idx >> r2_reg_idx;
    decode->csr_idx >> csr_idx;
    unk_checker->out >> illegal_inst;
  }

  SUBCOMPONENT(decode, TYPE(TrapDecode<XLEN>));
  SUBCOMPONENT(uncompress, TYPE(Uncompress<XLEN>));
  SUBCOMPONENT(unk_checker, TYPE(UNKChecker<XLEN>));

  INPUTPORT(instr, c_RVInstrWidth);
  OUTPUTPORT_ENUM(opcode, RVInstr);
  OUTPUTPORT(wr_reg_idx, c_RVRegsBits);
  OUTPUTPORT(r1_reg_idx, c_RVRegsBits);
  OUTPUTPORT(r2_reg_idx, c_RVRegsBits);
  OUTPUTPORT(Pc_Inc, 1);
  OUTPUTPORT(exp_instr, c_RVInstrWidth);
  OUTPUTPORT(illegal_inst, 1);
  OUTPUTPORT_ENUM(csr_idx, CSR);
};

} // namespace core
} // namespace vsrtl

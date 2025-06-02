#pragma once

#include "VSRTL/core/vsrtl_adder.h"
#include "VSRTL/core/vsrtl_constant.h"
#include "VSRTL/core/vsrtl_design.h"
#include "VSRTL/core/vsrtl_logicgate.h"
#include "VSRTL/core/vsrtl_multiplexer.h"
#include "rv_rti_adder.h"
#include "rv_rti_mux.h"
#include "rv_special_csrs.h"
#include "rv_ss_trap_mems.h"
#include "rv_trap_decoder.h"
#include "trap_checker.h"
#include "rv_ss_trap_control.h"
#include "rv_direction_mux.h"

#include "../../ripesvsrtlprocessor.h"

#include "../riscv.h"
#include "../rv_alu.h"
#include "../rv_branch.h"
#include "../rv_control.h"
#include "../rv_ecallchecker.h"
#include "../rv_immediate.h"
#include "../rv_memory.h"
#include "../rv_registerfile.h"
#include "rv_trap_decodeRVC.h"

namespace vsrtl {
namespace core {
using namespace Ripes;

template <typename XLEN_T>
class RVSS_TRAP : public RipesVSRTLProcessor {
  static_assert(std::is_same<uint32_t, XLEN_T>::value ||
                    std::is_same<uint64_t, XLEN_T>::value,
                "Only supports 32- and 64-bit variants");
  static constexpr unsigned XLEN = sizeof(XLEN_T) * CHAR_BIT;

public:
  RVSS_TRAP(const QStringList &extensions)
      : RipesVSRTLProcessor("Single Cycle RISC-V Processor with traps") {
    m_enabledISA = ISAInfoRegistry::getISA<XLenToRVISA<XLEN>()>(extensions);
    decode->setISA(m_enabledISA);

    // -----------------------------------------------------------------------
    // Interrupts
    trap_checker->ei >> *ei_and->in[0];
    trap_checker->si >> *si_and->in[0];
    trap_checker->ti >> *ti_and->in[0];
    trap_checker->ei >> mip_reg->eip_in;
    trap_checker->si >> mip_reg->sip_in;
    trap_checker->ti >> mip_reg->tip_in;
    trap_checker->ei >> trap_decoder->ei;
    trap_checker->si >> trap_decoder->si;
    trap_checker->ti >> trap_decoder->ti;
    0 >> trap_checker->dummy;

    // -----------------------------------------------------------------------
    // Interrupt enabled checker
    mie_reg->eie_out >> *ei_and->in[1];
    mie_reg->sie_out >> *si_and->in[1];
    mie_reg->tie_out >> *ti_and->in[1];

    ei_and->out >> *ie_or_1->in[0];
    si_and->out >> *ie_or_1->in[1];
    ie_or_1->out >> *ie_or_2->in[0];
    ti_and->out >> *ie_or_2->in[1];

    ie_or_2->out >> *ie_and->in[0];
    mstatus_reg->ie_out >> *ie_and->in[1];
    ie_and->out >> *trap_or->in[0];
    exception_or->out >> *trap_or->in[1];
    ie_and->out >> *rti_and->in[1];
    mtvec_reg->mode_out >> *rti_and->in[0];

    trap_decoder->out >> *mcause_src->ins[1];
    trap_or->out >> *mcause_enable_or->in[1];
    trap_or->out >> mcause_src->select;

    trap_or->out >> *mepc_enable_or->in[1];
    trap_or->out >> mepc_src->select;


    trap_or->out >> csr_op_ctrl->select;
    control->csr_op >> *csr_op_ctrl->ins[0];
    0 >> *csr_op_ctrl->ins[1];
    csr_op_ctrl->out >> mstatus_reg->c_s;
    csr_op_ctrl->out >> mtvec_reg->c_s;
    csr_op_ctrl->out >> mie_reg->c_s;
    csr_op_ctrl->out >> mepc_reg->c_s;
    csr_op_ctrl->out >> mip_reg->c_s;
    csr_op_ctrl->out >> mcause_reg->c_s;
    csr_op_ctrl->out >> mtval_reg->c_s;


    // -----------------------------------------------------------------------
    // Program counter
    pc_reg->out >> pc_4->op1;
    pc_inc->out >> pc_4->op2;
    pc_src->out >> *pc_src_2->ins[0];
    mepc_reg->out >> *pc_src_2->ins[1];
    control->isMret >> pc_src_2->select;
    pc_src_2->out >> *pc_src_3->ins[0];
    rti_mux->out >> *pc_src_3->ins[1];
    trap_or->out >> pc_src_3->select;
    pc_src_3->out >> pc_reg->in;

    2 >> pc_inc->get(PcInc::INC2);
    4 >> pc_inc->get(PcInc::INC4);
    decode->Pc_Inc >> pc_inc->select;

    // Note: pc_src works uses the PcSrc enum, but is selected by the boolean
    // signal from the controlflow OR gate. PcSrc enum values must adhere to the
    // boolean 0/1 values.
    controlflow_or->out >> pc_src->select;

    // -----------------------------------------------------------------------
    // Instruction memory
    pc_reg->out >> instr_mem->addr;
    instr_mem->setMemory(m_memory);
    instr_mem->pc_add_misaligned >> *exception_or->in[0];
    instr_mem->inst_acc_fault >> *exception_or->in[1];
    instr_mem->data_out >> decode->instr;
    instr_mem->pc_add_misaligned >> trap_decoder->pc_add_misaligned;
    instr_mem->inst_acc_fault >> trap_decoder->inst_acc_fault;

    // -----------------------------------------------------------------------
    // Decode
    decode->illegal_inst >> *exception_or->in[2];
    decode->illegal_inst >> trap_decoder->illegal_inst;
    decode->csr_idx >> control->csr_idx;

    // -----------------------------------------------------------------------
    // Control signals
    decode->opcode >> control->opcode;

    // -----------------------------------------------------------------------
    // Immediate
    decode->opcode >> immediate->opcode;
    decode->exp_instr >> immediate->instr;

    // -----------------------------------------------------------------------
    // Registers
    decode->wr_reg_idx >> registerFile->wr_addr;
    decode->r1_reg_idx >> registerFile->r1_addr;
    decode->r2_reg_idx >> registerFile->r2_addr;
    exception_or->out >> reg_wr_ctrl->select;
    control->reg_do_write_ctrl >> *reg_wr_ctrl->ins[0];
    0 >> *reg_wr_ctrl->ins[1];
    reg_wr_ctrl->out >> registerFile->wr_en;
    reg_wr_src->out >> registerFile->data_in;

    data_mem->data_out >> reg_wr_src->get(RegWrTrapSrc::MEMREAD);
    alu->res >> reg_wr_src->get(RegWrTrapSrc::ALURES);
    pc_4->out >> reg_wr_src->get(RegWrTrapSrc::PC4);
    control->reg_wr_src_ctrl >> reg_wr_src->select;

    registerFile->setMemory(m_regMem);

    // -----------------------------------------------------------------------
    // CSRs
    control->csr_mcause_en >> *mcause_enable_or->in[0];
    control->csr_mtvec_en >> mtvec_reg->enable;
    control->csr_mie_en >> mie_reg->enable;
    control->csr_mip_en >> mip_reg->enable;
    control->csr_mstatus_en >> mstatus_reg->enable;
    control->csr_mtval_en >> mtval_reg->enable;
    control->csr_mepc_en >> *mepc_enable_or->in[0];
    mepc_enable_or->out >> mepc_reg->enable;
    mcause_enable_or->out >> mcause_reg->enable;

    mstatus_reg->out >> *csr_data_src->ins[0];
    mtvec_reg->out >> *csr_data_src->ins[1];
    mie_reg->out >> *csr_data_src->ins[2];
    mepc_reg->out >> *csr_data_src->ins[3];
    mip_reg->out >> *csr_data_src->ins[4];
    mcause_reg->out >> *csr_data_src->ins[5];
    mtval_reg->out >> *csr_data_src->ins[6];
    csr_data_src->out >> reg_wr_src->get(RegWrTrapSrc::CSR);
    decode->csr_idx >> csr_data_src->select;

    0 >> mstatus_reg->clear;
    0 >> mtvec_reg->clear;
    0 >> mie_reg->clear;
    0 >> mepc_reg->clear;
    0 >> mip_reg->clear;
    0 >> mcause_reg->clear;
    0 >> mtval_reg->clear;

    control->csr_wr_select >> csr_wr_src->select;
    immediate->imm >> *csr_wr_src->ins[0]; 
    registerFile->r1_out >> *csr_wr_src->ins[1]; 
    csr_wr_src->out >> mstatus_reg->in;  
    csr_wr_src->out >> mtvec_reg->in;  
    csr_wr_src->out >> mie_reg->in;  
    mepc_src->out >> mepc_reg->in;
    csr_wr_src->out >> mip_reg->in;  
    mcause_src->out >> mcause_reg->in;
    csr_wr_src->out >> mtval_reg->in;  
    csr_wr_src->out >> *mepc_src->ins[0];  
    pc_reg->out >> *mepc_src->ins[1];
    csr_wr_src->out >> *mcause_src->ins[0];  

    trap_decoder->out >> mtval_reg->cause; 
    pc_reg->out >> mtval_reg->pc; 
    alu->res >> mtval_reg->mem_addr; 

    control->isMret >> mstatus_reg->isMret; 
    trap_or->out >> mstatus_reg->trap; 

    // -----------------------------------------------------------------------
    // Branch
    control->comp_ctrl >> branch->comp_op;
    registerFile->r1_out >> branch->op1;
    registerFile->r2_out >> branch->op2;

    branch->res >> *br_and->in[0];
    control->do_branch >> *br_and->in[1];
    br_and->out >> *controlflow_or->in[0];
    control->do_jump >> *controlflow_or->in[1];
    pc_4->out >> pc_src->get(PcSrc::PC4);
    alu->res >> pc_src->get(PcSrc::ALU);

    // -----------------------------------------------------------------------
    // ALU
    registerFile->r1_out >> alu_op1_src->get(AluSrc1::REG1);
    pc_reg->out >> alu_op1_src->get(AluSrc1::PC);
    control->alu_op1_ctrl >> alu_op1_src->select;

    registerFile->r2_out >> alu_op2_src->get(AluSrc2::REG2);
    immediate->imm >> alu_op2_src->get(AluSrc2::IMM);
    control->alu_op2_ctrl >> alu_op2_src->select;

    alu_op1_src->out >> alu->op1;
    alu_op2_src->out >> alu->op2;

    control->alu_ctrl >> alu->ctrl;

    // -----------------------------------------------------------------------
    // Data memory
    alu->res >> mem_dir_mux->alu_res;
    exception_or->out >> mem_wr_ctrl->select;
    control->mem_do_write_ctrl >> *mem_wr_ctrl->ins[0];
    0 >> *mem_wr_ctrl->ins[1];
    mem_wr_ctrl->out >> data_mem->wr_en;
    registerFile->r2_out >> data_mem->data_in;
    control->mem_ctrl >> data_mem->op;

    data_mem->mem->setMemory(m_memory);

    mem_hzrd_detector->setMemory(m_memory);
    alu->res >> mem_hzrd_detector->addr;
    control->mem_ctrl >> mem_hzrd_detector->op;
    mem_hzrd_detector->load_add_misaligned >> *exception_or->in[3];
    mem_hzrd_detector->store_add_misaligned >> *exception_or->in[4];
    mem_hzrd_detector->load_acc_fault >> *exception_or->in[5];
    mem_hzrd_detector->store_acc_fault >> *exception_or->in[6];
    mem_hzrd_detector->load_add_misaligned >> trap_decoder->load_add_misaligned;
    mem_hzrd_detector->store_add_misaligned >> trap_decoder->store_add_misaligned;
    mem_hzrd_detector->load_acc_fault >> trap_decoder->load_acc_fault;
    mem_hzrd_detector->store_acc_fault >> trap_decoder->store_acc_fault;

    control->mem_ctrl >> mem_dir_mux->select;
    0 >> mem_dir_mux->zero;
    mem_dir_mux->out >> data_mem->addr;

    // -----------------------------------------------------------------------
    // RTI Address calc
    mtvec_reg->out >> rti_adder->op1;
    mcause_reg->out >> rti_adder->op2;
    rti_adder->out >> rti_mux->op1;
    mtvec_reg->out >> rti_mux->op2;
    rti_and->out >> rti_mux->select;

    // -----------------------------------------------------------------------
    // Ecall checker
    decode->opcode >> ecallChecker->opcode;
    ecallChecker->setSyscallCallback(&trapHandler);
    0 >> ecallChecker->stallEcallHandling;
  }

  // Design subcomponents
  SUBCOMPONENT(registerFile, TYPE(RegisterFile<XLEN, false>));
  SUBCOMPONENT(alu, TYPE(ALU<XLEN>));
  SUBCOMPONENT(control, TrapControl);
  SUBCOMPONENT(immediate, TYPE(Immediate<XLEN>));
  SUBCOMPONENT(decode, TYPE(DecodeRVC_Trap<XLEN>));
  SUBCOMPONENT(branch, TYPE(Branch<XLEN>));
  SUBCOMPONENT(pc_4, Adder<XLEN>);
  SUBCOMPONENT(rti_adder, RTIAdder<XLEN>);

  // Registers
  SUBCOMPONENT(pc_reg, Register<XLEN>);

  // Multiplexers
  SUBCOMPONENT(csr_wr_src, TYPE(EnumMultiplexer<CSRWrSrc, XLEN>));
  SUBCOMPONENT(reg_wr_src, TYPE(EnumMultiplexer<RegWrTrapSrc, XLEN>));
  SUBCOMPONENT(pc_src, TYPE(EnumMultiplexer<PcSrc, XLEN>));
  SUBCOMPONENT(pc_src_2, TYPE(EnumMultiplexer<PcSrc2, XLEN>));
  SUBCOMPONENT(pc_src_3, TYPE(EnumMultiplexer<PcSrc3, XLEN>));
  SUBCOMPONENT(alu_op1_src, TYPE(EnumMultiplexer<AluSrc1, XLEN>));
  SUBCOMPONENT(alu_op2_src, TYPE(EnumMultiplexer<AluSrc2, XLEN>));
  SUBCOMPONENT(mepc_src, TYPE(EnumMultiplexer<MepcSrc, XLEN>));
  SUBCOMPONENT(mcause_src, TYPE(EnumMultiplexer<McauseSrc, XLEN>));
  SUBCOMPONENT(csr_data_src, TYPE(EnumMultiplexer<CSR, XLEN>));
  SUBCOMPONENT(rti_mux, TYPE(RTIMux<XLEN>));
  SUBCOMPONENT(pc_inc, TYPE(EnumMultiplexer<PcInc, XLEN>));
  SUBCOMPONENT(csr_op_ctrl, TYPE(Multiplexer<2,2>));
  SUBCOMPONENT(mem_wr_ctrl, TYPE(Multiplexer<2,1>));
  SUBCOMPONENT(reg_wr_ctrl, TYPE(Multiplexer<2,1>));

  // Memories
  SUBCOMPONENT(instr_mem, TYPE(InstrMemExcp<XLEN, c_RVInstrWidth>));
  SUBCOMPONENT(data_mem, TYPE(RVMemory<XLEN, XLEN>));
  SUBCOMPONENT(mem_hzrd_detector, TYPE(MemHzrdDetectionUnit<XLEN, XLEN>));
  SUBCOMPONENT(mem_dir_mux, TYPE(MemDirMux<XLEN>));

  // Gates
  SUBCOMPONENT(br_and, TYPE(And<1, 2>));
  SUBCOMPONENT(rti_and, TYPE(Or<1, 2>));
  SUBCOMPONENT(ei_and, TYPE(And<1, 2>));
  SUBCOMPONENT(si_and, TYPE(And<1, 2>));
  SUBCOMPONENT(ti_and, TYPE(And<1, 2>));
  SUBCOMPONENT(ie_and, TYPE(And<1, 2>));
  SUBCOMPONENT(ie_or_1, TYPE(Or<1, 2>));
  SUBCOMPONENT(ie_or_2, TYPE(Or<1, 2>));
  SUBCOMPONENT(controlflow_or, TYPE(Or<1, 2>));
  SUBCOMPONENT(trap_or, TYPE(Or<1, 2>));
  SUBCOMPONENT(mepc_enable_or, TYPE(Or<1, 2>));
  SUBCOMPONENT(mcause_enable_or, TYPE(Or<1, 2>));
  SUBCOMPONENT(exception_or, TYPE(Or<1, 7>));

  // CSRs
  SUBCOMPONENT(mstatus_reg, RegisterMSTATUS<XLEN>);
  SUBCOMPONENT(mtvec_reg, RegisterMTVEC<XLEN>);
  SUBCOMPONENT(mie_reg, RegisterMIE<XLEN>);
  SUBCOMPONENT(mepc_reg, RegisterClEnCS<XLEN>);
  SUBCOMPONENT(mip_reg, RegisterMIP<XLEN>);
  SUBCOMPONENT(mcause_reg, RegisterClEnCS<XLEN>);
  SUBCOMPONENT(mtval_reg, RegisterMTVAL<XLEN>);

  // Decoder traps
  SUBCOMPONENT(trap_decoder, TYPE(TrapDecoder<XLEN>));

  //Trap checker
  SUBCOMPONENT(trap_checker, TYPE(TrapChecker));

  // Address spaces
  ADDRESSSPACEMM(m_memory);
  ADDRESSSPACE(m_regMem);

  SUBCOMPONENT(ecallChecker, EcallChecker);

  // Ripes interface compliance
  const ProcessorStructure &structure() const override { return m_structure; }
  unsigned int getPcForStage(StageIndex) const override {
    return pc_reg->out.uValue();
  }
  AInt nextFetchedAddress() const override { return pc_src->out.uValue(); }
  QString stageName(StageIndex) const override { return "•"; }
  StageInfo stageInfo(StageIndex) const override {
    return StageInfo({pc_reg->out.uValue(),
                      isExecutableAddress(pc_reg->out.uValue()),
                      StageInfo::State::None});
  }
  void setProgramCounter(AInt address) override {
    pc_reg->forceValue(0, address);
    propagateDesign();
  }
  void setPCInitialValue(AInt address) override {
    pc_reg->setInitValue(address);
  }
  AddressSpaceMM &getMemory() override { return *m_memory; }
  VInt getRegister(const std::string_view &, unsigned i) const override {
    return registerFile->getRegister(i);
  }
  void finalize(FinalizeReason fr) override {
    if (fr == FinalizeReason::exitSyscall) {
      // Allow one additional clock cycle to clear the current instruction
      m_finishInNextCycle = true;
    }
  }
  bool finished() const override {
    return m_finished || !stageInfo({0, 0}).stage_valid;
  }
  const std::vector<StageIndex> breakpointTriggeringStages() const override {
    return {{0, 0}};
  }

  MemoryAccess dataMemAccess() const override {
    return memToAccessInfo(data_mem);
  }
  MemoryAccess instrMemAccess() const override {
    auto instrAccess = memToAccessInfo(instr_mem);
    instrAccess.type = MemoryAccess::Read;
    return instrAccess;
  }

  void setRegister(const std::string_view &, unsigned i, VInt v) override {
    setSynchronousValue(registerFile->_wr_mem, i, v);
  }

  void clockProcessor() override {
    // Single cycle processor; 1 instruction retired per cycle!
    m_instructionsRetired++;

    // m_finishInNextCycle may be set during Design::clock(). Store the value
    // before clocking the processor, and emit finished if this was the final
    // clock cycle.
    const bool finishInThisCycle = m_finishInNextCycle;
    Design::clock();
    if (finishInThisCycle) {
      m_finished = true;
    }
  }

  void reverse() override {
    m_instructionsRetired--;
    Design::reverse();
    // Ensure that reverses performed when we expected to finish in the
    // following cycle, clears this expectation.
    m_finishInNextCycle = false;
    m_finished = false;
  }

  void reset() override {
    Design::reset();
    m_finishInNextCycle = false;
    m_finished = false;
  }

  static ProcessorISAInfo supportsISA() { return RVISA::supportsISA<XLEN>(); }
  std::shared_ptr<ISAInfoBase> implementsISA() const override {
    return m_enabledISA;
  }
  std::shared_ptr<const ISAInfoBase> fullISA() const override {
    return RVISA::fullISA<XLEN>();
  }

  const std::set<std::string_view> registerFiles() const override {
    std::set<std::string_view> rfs;
    rfs.insert(RVISA::GPR);

    if (implementsISA()->extensionEnabled("F")) {
      rfs.insert(RVISA::FPR);
    }
    return rfs;
  }

  vsrtl::core::TrapChecker* getTrapChecker() override { return trap_checker; }


private:
  bool m_finishInNextCycle = false;
  bool m_finished = false;
  std::shared_ptr<ISAInfoBase> m_enabledISA;
  ProcessorStructure m_structure = {{0, 1}};
};

} // namespace core
} // namespace vsrtl

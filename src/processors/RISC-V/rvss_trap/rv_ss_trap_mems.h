#pragma once

#include <iostream>
#include "VSRTL/core/vsrtl_memory.h"
#include "processors/RISC-V/rv_memory.h"
#include "VSRTL/core/vsrtl_addressspace.h"

namespace vsrtl {
namespace core {
template <unsigned int addrWidth, unsigned int dataWidth, bool byteIndexed = true>
class InstrMemExcp : public ROM<addrWidth, dataWidth, byteIndexed> {
public:
  InstrMemExcp(const std::string& name, SimComponent* parent)
      : ROM<addrWidth, dataWidth, byteIndexed>(name, parent) {

    pc_add_misaligned << [=] {
      return (VSRTL_VT_U)((this->addr.uValue() & 0b11) ? 1 : 0);
    };

    inst_acc_fault << [=] {
      const VSRTL_VT_U maxValue = (VSRTL_VT_U(1) << dataWidth) - 1;
      return (this->addr.uValue() > maxValue) ? 1 : 0;
    };
  }

  // New Outputs
  OUTPUTPORT(pc_add_misaligned, 1);
  OUTPUTPORT(inst_acc_fault, 1);
};

template <unsigned int addrWidth, unsigned int dataWidth>
class DataMemExcp : public RVMemory<addrWidth, dataWidth> {
public:
  DataMemExcp(const std::string &name, SimComponent *parent)
      : RVMemory<addrWidth, dataWidth>(name, parent) {
    load_add_misaligned << [=] {
      unsigned int alignment = 1;
      switch (this->op.template eValue<MemOp>()) {
      case MemOp::LB:
      case MemOp::LBU:
        alignment = 1;
        break;
      case MemOp::LH:
      case MemOp::LHU:
        alignment = 2;
        break;
      case MemOp::LW:
      case MemOp::LWU:
        alignment = 4;
        break;
      case MemOp::LD:
        alignment = 8;
        break;
      default:
        alignment = 1;
        break;
      }
      return (this->addr.uValue() % alignment) ? 1 : 0;
    };
    store_add_misaligned << [=] {
      unsigned int alignment = 1;
      switch (this->op.template eValue<MemOp>()) {
      case MemOp::SB:
        alignment = 1;
        break;
      case MemOp::SH:
        alignment = 2;
        break;
      case MemOp::SW:
        alignment = 4;
        break;
      case MemOp::SD:
        alignment = 8;
        break;
      default:
        alignment = 1;
        break;
      }
      return (this->addr.uValue() % alignment) ? 1 : 0;
    };
    load_acc_fault << [=] {
      if (this->op.template eValue<MemOp>() == MemOp::LB || this->op.template eValue<MemOp>() == MemOp::LBU
        || this->op.template eValue<MemOp>() == MemOp::LH || this->op.template eValue<MemOp>() == MemOp::LHU
        || this->op.template eValue<MemOp>() == MemOp::LW || this->op.template eValue<MemOp>() == MemOp::LWU
        || this->op.template eValue<MemOp>() == MemOp::LD) {
        if (this->accessRegion() == AddressSpace::RegionType::Program) {
          const VSRTL_VT_U maxValue = (VSRTL_VT_U(1) << dataWidth) - 1;
          return (this->addr.uValue() > maxValue) ? 1 : 0;
        }
        }
      return 0;
    };
    store_acc_fault << [=] {
      if (this->op.template eValue<MemOp>() == MemOp::SB || this->op.template eValue<MemOp>() == MemOp::SD
       || this->op.template eValue<MemOp>() == MemOp::SH || this->op.template eValue<MemOp>() == MemOp::SW) {
        if (this->accessRegion() == AddressSpace::RegionType::Program) {
          const VSRTL_VT_U maxValue = (VSRTL_VT_U(1) << dataWidth) - 1;
          return (this->addr.uValue() > maxValue) ? 1 : 0;
        }
       }
      return 0;
    };
  }

  OUTPUTPORT(load_add_misaligned, 1);
  OUTPUTPORT(store_add_misaligned, 1);
  OUTPUTPORT(load_acc_fault, 1);
  OUTPUTPORT(store_acc_fault, 1);
};

template <unsigned int addrWidth, unsigned int dataWidth>
class MemHzrdDetectionUnit : public Component {
public:
  SetGraphicsType(Component);
  AddressSpaceMM *memory = nullptr;

  void setMemory(AddressSpaceMM *mem) {
    memory = mem;
  }

  MemHzrdDetectionUnit(const std::string& name, SimComponent* parent)
      : Component(name, parent) {
    load_add_misaligned << [=] {
      unsigned int alignment = 1;
      switch (op.eValue<MemOp>()) {
      case MemOp::LB:
      case MemOp::LBU:
        alignment = 1;
        break;
      case MemOp::LH:
      case MemOp::LHU:
        alignment = 2;
        break;
      case MemOp::LW:
      case MemOp::LWU:
        alignment = 4;
        break;
      case MemOp::LD:
        alignment = 8;
        break;
      default:
        alignment = 1;
        break;
      }
      return (addr.uValue() % alignment) ? 1 : 0;
    };
    store_add_misaligned << [=] {
      unsigned int alignment = 1;
      switch (op.eValue<MemOp>()) {
      case MemOp::SB:
        alignment = 1;
        break;
      case MemOp::SH:
        alignment = 2;
        break;
      case MemOp::SW:
        alignment = 4;
        break;
      case MemOp::SD:
        alignment = 8;
        break;
      default:
        alignment = 1;
        break;
      }
      return (addr.uValue() % alignment) ? 1 : 0;
    };
    load_acc_fault << [=] {
      if (op.eValue<MemOp>() == MemOp::LB || op.eValue<MemOp>() == MemOp::LBU
        || op.eValue<MemOp>() == MemOp::LH || op.eValue<MemOp>() == MemOp::LHU
        || op.eValue<MemOp>() == MemOp::LW || op.eValue<MemOp>() == MemOp::LWU
        || op.eValue<MemOp>() == MemOp::LD) {
        if (memory->regionType(addr.uValue()) == AddressSpace::RegionType::Program) {
          const VSRTL_VT_U maxValue = (VSRTL_VT_U(1) << dataWidth) - 1;
          return (addr.uValue() > maxValue) ? 1 : 0;
        }
        }
      return 0;
    };
    store_acc_fault << [=] {
      if (op.eValue<MemOp>() == MemOp::SB || op.eValue<MemOp>() == MemOp::SD
       || op.eValue<MemOp>() == MemOp::SH || op.eValue<MemOp>() == MemOp::SW) {
        if (memory->regionType(addr.uValue()) == AddressSpace::RegionType::Program) {
          const VSRTL_VT_U maxValue = (VSRTL_VT_U(1) << dataWidth) - 1;
          return (addr.uValue() > maxValue) ? 1 : 0;
        }
       }
      return 0;
    };
  }
  OUTPUTPORT(load_add_misaligned, 1);
  OUTPUTPORT(store_add_misaligned, 1);
  OUTPUTPORT(load_acc_fault, 1);
  OUTPUTPORT(store_acc_fault, 1);

  INPUTPORT(addr, addrWidth);
  INPUTPORT_ENUM(op, MemOp);
};


  }

}
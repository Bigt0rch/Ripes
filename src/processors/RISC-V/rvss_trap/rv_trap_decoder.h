#pragma once

#include "VSRTL/core/vsrtl_component.h"

#include "VSRTL/interface/vsrtl_gfxobjecttypes.h"

namespace vsrtl {
namespace core {

template <unsigned int XLEN>
class TrapDecoder : public Component {
public:
  SetGraphicsType(ClockedComponent);
  TrapDecoder(const std::string &name, SimComponent *parent)
      : Component(name, parent) {
    out << [=] {
      if (pc_add_misaligned.uValue() == 1) {
        return (VSRTL_VT_U) 0;
      }
      if (inst_acc_fault.uValue() == 1) {
        return (VSRTL_VT_U) 1;
      }
      if (illegal_inst.uValue() == 1) {
        return (VSRTL_VT_U) 2;
      }
      if (load_add_misaligned.uValue() == 1) {
        return (VSRTL_VT_U) 4;
      }
      if (load_acc_fault.uValue() == 1) {
        return (VSRTL_VT_U) 5;
      }
      if (store_add_misaligned.uValue() == 1) {
        return (VSRTL_VT_U) 6;
      }
      if (store_acc_fault.uValue() == 1) {
        return (VSRTL_VT_U) 7;
      }
      if (ti.uValue() == 1) {
        VSRTL_VT_U value = 7;
        value |= (1U << 31);
        return value;
      }
      if (ei.uValue() == 1) {
        VSRTL_VT_U value = 11;
        value |= (1U << 31);
        return value;
      }
      if (si.uValue() == 1) {
        VSRTL_VT_U value = 3;
        value |= (1U << 31);
        return value;
      }
      return (VSRTL_VT_U)0xDEADBEEF;
    };
  }

  INPUTPORT(store_acc_fault, 1);
  INPUTPORT(load_acc_fault, 1);
  INPUTPORT(store_add_misaligned, 1);
  INPUTPORT(load_add_misaligned, 1);
  INPUTPORT(illegal_inst, 1);
  INPUTPORT(inst_acc_fault,1);
  INPUTPORT(pc_add_misaligned, 1);
  INPUTPORT(ei, 1);
  INPUTPORT(ti, 1);
  INPUTPORT(si, 1);
  OUTPUTPORT(out, XLEN);
};
} // namespace core
} // namespace vsrtl
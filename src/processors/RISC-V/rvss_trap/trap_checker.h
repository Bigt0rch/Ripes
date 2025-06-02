#pragma once

#include "VSRTL/core/vsrtl_register.h"
#include "VSRTL/core/vsrtl_port.h"
#include "io/ioplic.h"

namespace vsrtl {
namespace core {

class TrapChecker : public ClockedComponent {
private:
  Ripes::IOPLIC * m_plic = nullptr;
public:
  SetGraphicsType(ClockedComponent);

  TrapChecker(const std::string &name, SimComponent *parent)
      : ClockedComponent(name, parent) {
    ei << [this] {
      if (m_plic == nullptr) return 0;
      if (m_plic->hasPending()) return 1;
      return 0;
    };
    si << [=] { return 0; }; //TODO: check for real interrupts
    ti << [=] { return 0; }; //TODO: check for real interrupts
  }
  OUTPUTPORT(ti, 1);
  OUTPUTPORT(si, 1);
  OUTPUTPORT(ei, 1);
  INPUTPORT(dummy,1);
  void setPLIC(Ripes::IOPLIC *p) { // can receive nullptr
    m_plic = p;
  }

  bool externalInterrupts() {
    if (m_plic == nullptr) return false;
    return m_plic->hasPending();
  }


  void reset() override {}

  void reverse() override {}

  void forceValue(VSRTL_VT_U addr, VSRTL_VT_U value) override {}

  void save() override {}

  void reverseStackSizeChanged() override {}
};


} // namespace core
} // namespace vsrtl

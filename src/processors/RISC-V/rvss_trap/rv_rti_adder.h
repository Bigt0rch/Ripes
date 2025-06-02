#pragma once

namespace vsrtl {
namespace core {
template <unsigned int XLEN>
class RTIAdder : public Component {
public:
  RTIAdder(const std::string &name, SimComponent *parent)
      : Component(name, parent) {
    this->out << [=] {
      VSRTL_VT_U mtvec_val = this->op1.uValue();
      VSRTL_VT_U cause_val = this->op2.uValue();
      VSRTL_VT_U aligned = mtvec_val & ~0x3;
      VSRTL_VT_U shifted = cause_val << 2;
      return aligned + shifted;
    };
  }

  INPUTPORT(op1, XLEN);
  INPUTPORT(op2, XLEN);
  OUTPUTPORT(out, XLEN);
};
}
}
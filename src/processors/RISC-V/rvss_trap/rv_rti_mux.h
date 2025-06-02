#include "VSRTL/core/vsrtl_component.h"
#include "VSRTL/core/vsrtl_port.h"

namespace vsrtl {
namespace core {

template <unsigned int XLEN>
class RTIMux : public Component {
public:
  SetGraphicsType(Multiplexer);

  RTIMux(const std::string &name, SimComponent *parent)
      : Component(name, parent) {
    setSpecialPort(GFX_MUX_SELECT, &select);

    out << [=] {
      VSRTL_VT_U raw_op2 = op2.uValue();
      VSRTL_VT_U aligned_op2 = raw_op2 & ~0x3;
      return select.uValue() ? aligned_op2 : op1.uValue();
    };
  }


  INPUTPORT(select, 1);
  INPUTPORT(op1, XLEN);
  INPUTPORT(op2, XLEN);
  OUTPUTPORT(out, XLEN);
};

} // namespace core
} // namespace vsrtl

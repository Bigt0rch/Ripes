#include "VSRTL/core/vsrtl_component.h"
#include "VSRTL/core/vsrtl_port.h"
#include "../riscv.h"

namespace vsrtl {
namespace core {

template <unsigned int XLEN>
class MemDirMux : public Component {
public:
  SetGraphicsType(Component);

  MemDirMux(const std::string &name, SimComponent *parent)
      : Component(name, parent) {

    out << [=] {
      if (select.eValue<Ripes::MemOp>() == Ripes::MemOp::NOP)
        return zero.uValue();
      return alu_res.uValue();
    };
  }


  INPUTPORT_ENUM(select, Ripes::MemOp);
  INPUTPORT(alu_res, XLEN);
  INPUTPORT(zero, XLEN);
  OUTPUTPORT(out, XLEN);
};

} // namespace core
} // namespace vsrtl

#include "assembler.h"

namespace Ripes {
namespace Assembler {

std::shared_ptr<AssemblerBase>
constructAssemblerDynamic(std::shared_ptr<ISAInfoBase> isa) {
  if (auto rv32isa = std::dynamic_pointer_cast<ISAInfo<ISA::RV32I>>(isa)) {
    return std::make_shared<ISA_Assembler<ISA::RV32I>>(rv32isa);
  } else if (auto rv64isa =
                 std::dynamic_pointer_cast<ISAInfo<ISA::RV64I>>(isa)) {
    return std::make_shared<ISA_Assembler<ISA::RV64I>>(rv64isa);
  }

  throw std::runtime_error(
      std::string("Cannot dynamically construct assembler for isa: ") +
      isa->name().toStdString());
  return {};
}

} // namespace Assembler
} // namespace Ripes

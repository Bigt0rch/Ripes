#include "iomanager.h"

#include "processorhandler.h"
#include "ripessettings.h"

#include <memory>
#include <ostream>

#include "VSRTL/core/vsrtl_addressspace.h"

namespace Ripes {

IOManager::IOManager() : QObject(nullptr) {
  // Always re-register the currently active peripherals when the processor
  // changes
  connect(ProcessorHandler::get(), &ProcessorHandler::processorChanged, this,
          &IOManager::refreshAllPeriphsToProcessor);
  connect(ProcessorHandler::get(), &ProcessorHandler::processorReset, this,
          &IOManager::refreshMemoryMap);

  refreshMemoryMap();
}

QString IOManager::cSymbolsHeaderpath() const {
  if (m_symbolsHeaderFile) {
    return m_symbolsHeaderFile->fileName();
  } else {
    return QString();
  }
}

void IOManager::reset() {
  for (auto &device : m_peripherals) {
    device->reset();
    device->update();
  }
}

AInt IOManager::nextPeripheralAddress() const {
  AInt base = 0;
  if (m_periphMMappings.empty()) {
    base = static_cast<unsigned>(
        RipesSettings::value(RIPES_SETTING_PERIPHERALS_START).toInt());
  } else {
    for (const auto &periph : m_periphMMappings) {
      if (periph.second.end() > base) {
        base = periph.second.end();
      }
    }
  }
  return base;
}

AInt IOManager::assignBaseAddress(IOBase *peripheral) {
  unregisterPeripheralWithProcessor(peripheral);
  const AInt base = nextPeripheralAddress();
  m_periphMMappings[peripheral] = {base, peripheral->byteSize(),
                                   peripheral->name()};
  registerPeripheralWithProcessor(peripheral);
  return base;
}

void IOManager::assignBaseAddresses() {
  // First unassign all base addresses to start with a clean address map
  for (const auto &periph : m_peripherals) {
    unregisterPeripheralWithProcessor(periph);
  }
  for (const auto &periph : m_peripherals) {
    assignBaseAddress(periph);
  }
  refreshMemoryMap();
}

void IOManager::peripheralSizeChanged(IOBase *) {
  assignBaseAddresses();
  refreshMemoryMap();
}

void IOManager::registerPeripheralWithProcessor(IOBase *peripheral) {
  ProcessorHandler::getMemory().addIORegion(
      m_periphMMappings.at(peripheral).startAddr, peripheral->byteSize(),
      vsrtl::core::IOFunctors{
          [peripheral](AInt offset, VInt value, unsigned size) {
            peripheral->ioWrite(offset, value, size);
          },
          [peripheral](AInt offset, unsigned size) {
            return peripheral->ioRead(offset, size);
          },
        [peripheral](AInt offset, unsigned size) {
            return peripheral->ioReadConst(offset, size);
          }
      });

  peripheral->memWrite = [](AInt address, VInt value, unsigned size) {
    ProcessorHandler::getMemory().writeMem(address, value, size);
  };
  peripheral->memRead = [](AInt address, unsigned size) {
    return ProcessorHandler::getMemory().readMem(address, size);
  };
}

void IOManager::unregisterPeripheralWithProcessor(IOBase *peripheral) {
  const auto &mmEntry = m_periphMMappings.find(peripheral);
  if (mmEntry != m_periphMMappings.end()) {
    ProcessorHandler::getMemory().removeIORegion(mmEntry->second.startAddr,
                                                 mmEntry->second.size);
    m_periphMMappings.erase(mmEntry);
  }
}

IOBase* IOManager::createPeripheral(IOType type, unsigned forcedId) {
  // --- 1) Validación PLIC ---
  if (type == IOType::PLIC) {
    auto* tc = ProcessorHandler::getTrapChecker();
    if (!tc) {
      QMessageBox::warning(nullptr, "PLIC no soportado",
          "El procesador actual no soporta interrupciones.");
      return nullptr;
    }
    if (m_plic) {
      QMessageBox::information(nullptr, "PLIC duplicado",
          "Ya existe un PLIC instanciado.");
      return nullptr;
    }
  }

  // Create peripheral
  auto *peripheral = IOFactories.at(type)(nullptr);

  connect(peripheral, &IOBase::sizeChanged, this,
          [=] { peripheralSizeChanged(peripheral); });
  connect(peripheral, &IOBase::aboutToDelete, this,
          [=](std::atomic<bool>& ok) { removePeripheral(peripheral, ok); });

  if (forcedId != UINT_MAX) {
    peripheral->setID(forcedId);
  }


  // Rigister PLIC if it's the pype of peripheral we are creating
  if (type == IOType::PLIC) {
    m_plic = static_cast<IOPLIC*>(peripheral);
    // Connnect all existing peripherals
    connectPeripheralsToPLIC();
  }

  // if it supports interrupts & a PLIC already exists, we register it in the PLIC
  if (type != IOType::PLIC && peripheral->supportsInterrupts()) {
    if (!m_plic) {
      QMessageBox::warning(nullptr, "Periférico no soportado",
          peripheral->name() + " no puede generar IRQ sin PLIC.");
      return nullptr;
    }
    // Assign a globalID to the peripheral
    peripheral->setGlobalID(getNextGlobalId());
    // Register peripheral at PLIC
    m_plic->registerSource(peripheral->globalID(), peripheral);
  }

  m_peripherals.insert(peripheral);
  assignBaseAddress(peripheral);
  refreshMemoryMap();

  // If it's a PLIC, pass reference to trap_checker
  if (type == IOType::PLIC) {
    if (auto* tc = ProcessorHandler::getTrapChecker())
      tc->setPLIC(m_plic);
  }

  return peripheral;
}




void IOManager::removePeripheral(IOBase *peripheral, std::atomic<bool> &ok) {
  // If we are deleting a PLIC, remove the reference passed to trap_checker
  if (peripheral == m_plic) {
    if (auto* tc = ProcessorHandler::getTrapChecker())
      tc->setPLIC(nullptr);
    m_plic = nullptr;
    // Delete al peripheral refrences the PLIC posseses
    disconnectPeripheralsFromPLIC();
  }

  auto it = m_peripherals.find(peripheral);
  Q_ASSERT(it != m_peripherals.end());
  unsigned gid = peripheral->globalID();

  // Delete this peripheral reference from the PLIC
  if (peripheral->supportsInterrupts() && m_plic) {
    m_plic->unregisterSource(gid);
  }

  // 4) Elimina del manager
  unregisterPeripheralWithProcessor(peripheral);
  m_peripherals.erase(it);
  m_usedGlobalIds.erase(gid);

  ok = true;
}

void IOManager::connectPeripheralsToPLIC() {
  if (!m_plic) return;
  for (auto* p : m_peripherals) {
    if (p != m_plic && p->supportsInterrupts()) {
      m_plic->registerSource(p->globalID(), p);
    }
  }
}

void IOManager::disconnectPeripheralsFromPLIC() {
  if (!m_plic) {
    // Si no hay PLIC activo, simplemente limpia los registros internos
    return;
  }
  for (auto* p : m_peripherals) {
    if (p->supportsInterrupts()) {
      m_plic->unregisterSource(p->globalID());
    }
  }
}

void IOManager::refreshAllPeriphsToProcessor() {
  disconnectPeripheralsFromPLIC(); 

  // If a PLIC already exists, we deleted since the new processor might not suppport interrupts
  if (m_plic) { 
    std::atomic<bool> ok{false}; 
    removePeripheral(m_plic, ok); 
  } 

  for (const auto &periph : m_periphMMappings) {
    registerPeripheralWithProcessor(periph.first);
  }
}

void IOManager::refreshMemoryMap() {
  m_memoryMap.clear();

  for (const auto &periph : m_periphMMappings) {
    m_memoryMap[periph.second.startAddr] = periph.second;
  }

  const auto &program = ProcessorHandler::getProgram();
  if (program) {
    for (const auto &section : program.get()->sections) {
      m_memoryMap[section.second.address] =
          MemoryMapEntry{section.second.address,
                         static_cast<unsigned>(section.second.data.size()),
                         section.second.name};
    }
  }

  updateSymbols();
  emit memoryMapChanged();
}

std::vector<std::pair<Symbol, AInt>>
IOManager::assemblerSymbolsForPeriph(IOBase *peripheral) const {
  const QString &periphName = cName(peripheral->name());
  std::vector<std::pair<Symbol, AInt>> symbols;
  const auto &periphInfo = m_periphMMappings.at(peripheral);
  symbols.push_back(
      {{periphName + "_BASE", Symbol::Type::Address}, periphInfo.startAddr});
  symbols.push_back({periphName + "_SIZE", periphInfo.size});

  for (const auto &reg : peripheral->registers()) {
    if (reg.exported) {
      const QString base = periphName + "_" + cName(reg.name);
      const QString offset = base + "_OFFSET";
      symbols.push_back({offset, reg.address});
      symbols.push_back({base, reg.address + periphInfo.startAddr});
    }
  }

  if (auto *extraSymbols = peripheral->extraSymbols()) {
    for (const auto &extraSymbol : *extraSymbols) {
      symbols.push_back(
          {periphName + "_" + cName(extraSymbol.name), extraSymbol.value});
    }
  }

  return symbols;
}

void IOManager::updateSymbols() {
  m_assemblerSymbols.clear();

  // Generate symbol mapping + header file
  QStringList headerfile;
  headerfile << "#ifndef RIPES_IO_HEADER";
  headerfile << "#define RIPES_IO_HEADER";
  for (const auto &p : m_periphMMappings) {
    const QString &periphName = cName(p.first->name());
    headerfile << "// "
                  "************************************************************"
                  "*****************";
    headerfile << "// * " + periphName;
    headerfile << "// "
                  "************************************************************"
                  "*****************";

    auto symbols = assemblerSymbolsForPeriph(p.first);
    m_assemblerSymbols.abs.insert(symbols.begin(), symbols.end());

    for (const auto &symbol : assemblerSymbolsForPeriph(p.first)) {
      headerfile << "#define " + symbol.first.v + "\t" + "(0x" +
                        QString::number(symbol.second, 16) + ")";
    }

    headerfile << "\n";
  }
  headerfile << "#endif // RIPES_IO_HEADER";

  // Store header file at a temporary location
  if (!(m_symbolsHeaderFile &&
        (QFile::exists(m_symbolsHeaderFile->fileName())))) {
    m_symbolsHeaderFile = std::make_unique<QFile>(
        QDir::tempPath() + QDir::separator() + "ripes_system.h");
  }

  if (m_symbolsHeaderFile->open(QIODevice::ReadWrite | QIODevice::Truncate)) {
    m_symbolsHeaderFile->write(headerfile.join('\n').toUtf8());
    m_symbolsHeaderFile->close();
  }
}

unsigned IOManager::getNextGlobalId() { 
  for (unsigned cand = 1; cand <= 1023; ++cand) { 
    if (!m_usedGlobalIds.count(cand)) { 
      m_usedGlobalIds.insert(cand);
      return cand; 
    } 
  }
  qFatal("Se han agotado los IDs globales (1..1023)."); 
  return 0; // unreachable code 
}

} // namespace Ripes

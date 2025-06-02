#pragma once

#include "iobase.h"
#include <vector>
#include <set>

namespace Ripes {

/**
 *  Platform-Level Interrupt Controller
 *  Solo contexto 0, 1023 fuentes de interrupción.
 */
class IOPLIC : public IOBase {
  Q_OBJECT
public:
  explicit IOPLIC(QWidget *parent);
  ~IOPLIC() { unregister(); };

  /* --------- Interface IOBase --------- */
  QString baseName() const override            { return "PLIC"; }
  QString description() const override;
  unsigned byteSize()  const override          { return 0x200008; }   // 2 MiB + 8
  const std::vector<RegDesc>& registers() const override { return m_regDescs; }

  VInt ioRead (AInt offset, unsigned size) override;
  VInt ioReadConst (AInt offset, unsigned size) override;
  void ioWrite(AInt offset, VInt value, unsigned size) override;
  void reset() override;

  bool hasPending();                  // IRQ prio > threshold?

  void registerSource(unsigned id, IOBase* src);
  void unregisterSource(unsigned id);
protected:

  void parameterChanged(unsigned ID) override {}

private:
  /* register layout */
  static constexpr AInt PRIO_BASE   = 0x000000;
  static constexpr AInt PEND_BASE   = 0x001000;
  static constexpr AInt ENAB_BASE   = 0x002000;
  static constexpr AInt THRESH_REG  = 0x200000;
  static constexpr AInt CLAIM_REG   = 0x200004;

  static constexpr unsigned NSOURCES = 1024;       // 0 isn't used
  static constexpr unsigned WORDS    = NSOURCES / 32;

  std::unordered_map<unsigned, IOBase*> m_sources;

  std::vector<uint32_t> m_priority;
  std::vector<uint32_t> m_pending;
  std::vector<uint32_t> m_enabled;
  uint32_t              m_threshold = 0;
  std::set<unsigned>    m_served;

  std::vector<RegDesc> m_regDescs;
  void initRegDescs();

  unsigned claim();
  unsigned claimConst() const;
  void complete(unsigned sourceID);
};

} // namespace Ripes
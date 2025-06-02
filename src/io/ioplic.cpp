#include "ioplic.h"
#include "ioregistry.h"
#include <algorithm>
#include <cassert>

namespace Ripes {

IOPLIC::IOPLIC(QWidget *parent)
    : IOBase(IOType::PLIC, parent),
      m_priority(NSOURCES, 0),
      m_pending(WORDS, 0),
      m_enabled(WORDS, 0) {
    initRegDescs();
}

QString IOPLIC::description() const {
    return "RISC-V Platform-Level Interrupt Controller (context 0). "
           "Registros de prioridad, pendientes, enable, threshold y "
           "claim/complete.";
}

void IOPLIC::reset() {
    std::fill(m_priority.begin(), m_priority.end(), 0);
    std::fill(m_pending.begin(),  m_pending.end(), 0);
    std::fill(m_enabled.begin(),  m_enabled.end(), 0);
    m_threshold = 0;
    m_served.clear();
}

bool IOPLIC::hasPending() {
  // Update m_pending reading interruptPending() from each peripheral
  for (auto& [id, src] : m_sources) {
    unsigned w = id / 32, b = id % 32;
    bool wants = src->interruptPending();
    bool wasPending = (m_pending[w] >> b) & 1u;
    if (wants && !wasPending) {
      m_pending[w] |= (1u << b);
    }
  }

    for (unsigned i = 1; i < NSOURCES; ++i) {
        unsigned w = i / 32;
        unsigned b = i % 32;
        bool pen = (m_pending[w] >> b) & 1u;
        bool ena = (m_enabled[w] >> b) & 1u;
        if (ena && pen && m_priority[i] > m_threshold)
            return true;
    }
    return false;
}

unsigned IOPLIC::claim() {
    uint32_t bestPrio = 0;
    unsigned bestSrc  = 0;
    for (unsigned i = 1; i < NSOURCES; ++i) {
        unsigned w = i / 32;
        unsigned b = i % 32;
        bool pen = (m_pending[w] >> b) & 1u;
        bool ena = (m_enabled[w] >> b) & 1u;
        if (ena && pen && m_priority[i] > m_threshold && m_priority[i] > bestPrio) {
            bestPrio = m_priority[i];
            bestSrc  = i;
        }
    }
    if (bestSrc) {
        m_served.insert(bestSrc);
        unsigned w = bestSrc / 32;
        unsigned b = bestSrc % 32;
        m_pending[w] &= ~(1u << b);
    }
    return bestSrc;
}

unsigned IOPLIC::claimConst() const {
  uint32_t bestPrio = 0;
  unsigned bestSrc  = 0;
  for (unsigned i = 1; i < NSOURCES; ++i) {
    unsigned w = i / 32;
    unsigned b = i % 32;
    bool pen = (m_pending[w] >> b) & 1u;
    bool ena = (m_enabled[w] >> b) & 1u;
    if (ena && pen && m_priority[i] > m_threshold && m_priority[i] > bestPrio) {
      bestPrio = m_priority[i];
      bestSrc  = i;
    }
  }
  return bestSrc;
}

void IOPLIC::registerSource(unsigned id, IOBase* src) {
  m_sources[id] = src;
}

void IOPLIC::unregisterSource(unsigned id) {
  m_sources.erase(id);
}

void IOPLIC::complete(unsigned src) {
    m_served.erase(src);
}

VInt IOPLIC::ioRead(AInt off, unsigned /*size*/) {
  VInt value = 0;

  // Priorities (4 bytes pero peripheral)
  if (off >= PRIO_BASE && off < PEND_BASE) {
    unsigned id = (off - PRIO_BASE) >> 2;
    value = (id < NSOURCES) ? m_priority[id] : 0;
  }
  // Pending: 32 peripherals/word
  else if (off >= PEND_BASE && off < ENAB_BASE) {
    unsigned word = (off - PEND_BASE) >> 2;
    value = (word < WORDS) ? m_pending[word] : 0;
  }
  // Enable
  else if (off >= ENAB_BASE && off < THRESH_REG) {
    unsigned word = (off - ENAB_BASE) >> 2;
    value = (word < WORDS) ? m_enabled[word] : 0;
  }
  // Threshold
  else if (off == THRESH_REG) {
    value = m_threshold;
  }
  // Claim
  else if (off == CLAIM_REG) {
    value = claim();
  }
  else {
    value = 0;
  }

  return value;
}

// Interface access
VInt IOPLIC::ioReadConst(AInt off, unsigned /*size*/) {
  VInt value = 0;

  // Priorities (4 bytes pero peripheral)
  if (off >= PRIO_BASE && off < PEND_BASE) {
    unsigned id = (off - PRIO_BASE) >> 2;
    value = (id < NSOURCES) ? m_priority[id] : 0;
  }
  // Pending: 32 peripherals/word
  else if (off >= PEND_BASE && off < ENAB_BASE) {
    unsigned word = (off - PEND_BASE) >> 2;
    value = (word < WORDS) ? m_pending[word] : 0;
  }
  // Enable
  else if (off >= ENAB_BASE && off < THRESH_REG) {
    unsigned word = (off - ENAB_BASE) >> 2;
    value = (word < WORDS) ? m_enabled[word] : 0;
  }
  // Threshold
  else if (off == THRESH_REG) {
    value = m_threshold;
  }
  // Claim
  else if (off == CLAIM_REG) {
    value = claimConst();
  }
  else {
    value = 0;
  }

  return value;
}

void IOPLIC::ioWrite(AInt off, VInt val, unsigned /*size*/) {
  // Priorities
    if (off >= PRIO_BASE && off < PEND_BASE) {
        unsigned id = (off - PRIO_BASE) >> 2;
        if (id < NSOURCES)
            m_priority[id] = static_cast<uint32_t>(val);
        return;
    }
    // Enable bits
    if (off >= ENAB_BASE && off < THRESH_REG) {
        unsigned word = (off - ENAB_BASE) >> 2;
        if (word < WORDS)
            m_enabled[word] = static_cast<uint32_t>(val);
        return;
    }
    // Threshold
    if (off == THRESH_REG) {
        m_threshold = static_cast<uint32_t>(val);
        return;
    }
    // Complete via write to CLAIM
    if (off == CLAIM_REG) {
        complete(static_cast<unsigned>(val));
        return;
    }
}

// Register table (GUI + símbolos)
void IOPLIC::initRegDescs() {
  m_regDescs = {
    {"PRIO",     RegDesc::RW::RW, NSOURCES,    PRIO_BASE,  false},
    {"PEND_0",   RegDesc::RW::R,   WORDS,       PEND_BASE,  false},
    {"ENABLE_0", RegDesc::RW::RW,  WORDS,       ENAB_BASE,  false},
    {"THRESH",   RegDesc::RW::RW,  32,          THRESH_REG, true},
    {"CLAIM",    RegDesc::RW::RW,  32,          CLAIM_REG,  true}
  };
}


} // namespace Ripes

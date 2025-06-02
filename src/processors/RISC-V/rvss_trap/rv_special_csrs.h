#pragma once

#include "../riscv.h"
#include "VSRTL/core/vsrtl_component.h"
#include "VSRTL/core/vsrtl_register.h"

namespace vsrtl {
namespace core {
using namespace Ripes;

template <unsigned int XLEN>
class RegisterClEnCS : public RegisterClEn<XLEN> {
public:
  RegisterClEnCS(const std::string &name, SimComponent *parent)
      : RegisterClEn<XLEN>(name, parent) { }

  void save() override {
    this->saveToStack();

    if (this->clear.uValue() != 0) {
      this->m_savedValue = 0;
      return;
    }

    if (this->enable.uValue() == 1) {
      if (c_s.uValue() == 0) {
        // Read-Write
        this->m_savedValue = this->in.uValue();
      }
      else if (c_s.uValue() == 1) {
        // Read-Clear
        this->m_savedValue &= ~this->in.uValue();
      }
      else if (c_s.uValue() == 2) {
        // Read-Set
        this->m_savedValue |= this->in.uValue();
      }
    }

  }


  INPUTPORT(c_s, 2);
};

template <unsigned int XLEN>
class RegisterMIE : public RegisterClEnCS<XLEN> {
public:
  RegisterMIE(const std::string &name, SimComponent *parent)
      : RegisterClEnCS<XLEN>(name, parent) {
    sie_out << [=]{ return (this->m_savedValue >> 3) & 0x1; };
    tie_out << [=] { return (this->m_savedValue >> 7) & 0x1; };
    eie_out << [=] { return (this->m_savedValue >> 11) & 0x1; };
  }

  // Individual outputs
  OUTPUTPORT(sie_out, 1);
  OUTPUTPORT(tie_out, 1);
  OUTPUTPORT(eie_out, 1);
};

template <unsigned int XLEN>
class RegisterMIP : public RegisterClEnCS<XLEN> {
public:
  RegisterMIP(const std::string &name, SimComponent *parent)
      : RegisterClEnCS<XLEN>(name, parent) {}

  void save() override {
    this->saveToStack();

    if (this->clear.uValue() != 0) {
      this->m_savedValue = 0;
      return;
    }

    if (this->enable.uValue() == 1) {
      if (this->c_s.uValue() == 0) {
        // Read-Write
        this->m_savedValue = this->in.uValue();
      }
      else if (this->c_s.uValue() == 1) {
        // Read-Clear
        this->m_savedValue &= ~this->in.uValue();
      }
      else if (this->c_s.uValue() == 2) {
        // Read-Set
        this->m_savedValue |= this->in.uValue();
      }
    }

    // Overwrite interrupt pending bits if needed
    this->m_savedValue &= ~(1 << 3);
    this->m_savedValue |= (sip_in.uValue() & 0x1) << 3;

    this->m_savedValue &= ~(1 << 7);
    this->m_savedValue |= (tip_in.uValue() & 0x1) << 7;

    this->m_savedValue &= ~(1 << 11);
    this->m_savedValue |= (eip_in.uValue() & 0x1) << 11;
  }

  // Individual inputs
  INPUTPORT(sip_in, 1);
  INPUTPORT(tip_in, 1);
  INPUTPORT(eip_in, 1);
};

template <unsigned int XLEN>
class RegisterMSTATUS : public RegisterClEnCS<XLEN> {
public:
  RegisterMSTATUS(const std::string &name, SimComponent *parent)
      : RegisterClEnCS<XLEN>(name, parent) {
    ie_out << [=]{ return (this->m_savedValue >> 3) & 0x1; };
  }

  void save() override {
    this->saveToStack();

    this->saveToStack();

    if (this->clear.uValue() != 0) {
      this->m_savedValue = 0;
      return;
    }

    if (this->enable.uValue() == 1) {
      if (this->c_s.uValue() == 0) {
        // Read-Write
        this->m_savedValue = this->in.uValue();
      }
      else if (this->c_s.uValue() == 1) {
        // Read-Clear
        this->m_savedValue &= ~this->in.uValue();
      }
      else if (this->c_s.uValue() == 2) {
        // Read-Set
        this->m_savedValue |= this->in.uValue();
      }
    }

    if (isMret.uValue()) {
      // Deletes MIE value
      this->m_savedValue &= ~(1 << 3);

      // Copy MPIE to MIE
      this->m_savedValue |= ((this->m_savedValue >> 7) & 0x1) << 3;
    }
    else if (((this->m_savedValue >> 3) & 0x1) == 1 && trap.uValue() == 1) {
      // MIE
      uint8_t bit3 = (this->m_savedValue >> 3) & 0x1;

      // MIE = 0
      this->m_savedValue &= ~(1 << 3);

      // MPIE = MIE
      this->m_savedValue = (this->m_savedValue & ~(1 << 7)) | (bit3 << 7);
    }
  }

  OUTPUTPORT(ie_out, 1);

  INPUTPORT(trap, 1);
  INPUTPORT(isMret, 1);

};

template <unsigned int XLEN>
class RegisterMTVEC : public RegisterClEnCS<XLEN> {
public:
  RegisterMTVEC(const std::string &name, SimComponent *parent)
      : RegisterClEnCS<XLEN>(name, parent) {
    mode_out << [=] { return (this->m_savedValue >> 0) & 0x1; };
  }

  OUTPUTPORT(mode_out, 1);
};

template <unsigned int XLEN>
class RegisterMTVAL : public RegisterClEnCS<XLEN> {
public:
  RegisterMTVAL(const std::string &name, SimComponent *parent)
      : RegisterClEnCS<XLEN>(name, parent) {}

  void save() override {
    this->saveToStack();

    if (this->clear.uValue() != 0) {
      this->m_savedValue = 0;
      return;
    }

    if (this->enable.uValue() == 1) {
      if (this->c_s.uValue() == 0) {
        // Read-Write
        this->m_savedValue = this->in.uValue();
      }
      else if (this->c_s.uValue() == 1) {
        // Read-Clear
        this->m_savedValue &= ~this->in.uValue();
      }
      else if (this->c_s.uValue() == 2) {
        // Read-Set
        this->m_savedValue |= this->in.uValue();
      }
    }

    if (cause.uValue() == 0 || cause.uValue() == 1) {
      this->m_savedValue = pc.uValue();
      return;
    }
    else if (cause.uValue() == 4 || cause.uValue() == 5 ||
             cause.uValue() == 6 || cause.uValue() == 7) {
      this->m_savedValue = mem_addr.uValue();
      return;
    }
  }


  INPUTPORT(pc, XLEN);
  INPUTPORT(mem_addr, XLEN);
  INPUTPORT(cause, XLEN);
};

}
}
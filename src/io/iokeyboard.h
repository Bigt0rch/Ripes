#pragma once

#include "iobase.h"
#include <deque>

namespace Ripes {

/**
 * @brief Keyboard periférico: almacena hasta 4 caracteres en FIFO y genera
 *        una interrupción en cada pulsación de tecla.
 */
class IOKeyboard : public IOBase {
  Q_OBJECT
public:
  explicit IOKeyboard(QWidget *parent);
  ~IOKeyboard() { unregister(); };

  /* --------- Interface IOBase --------- */
  QString baseName() const override { return "Keyboard"; }
  QString description() const override {
    return "Teclado: escribe caracteres en la caja de texto y léelos uno a uno.";
  }
  unsigned byteSize() const override { return 0x04; }
  const std::vector<RegDesc>& registers() const override { return m_regDescs; }
  bool supportsInterrupts() const override { return true; }

  VInt ioRead(AInt offset, unsigned size) override;
  VInt ioReadConst(AInt offset, unsigned size) override;
  void ioWrite(AInt offset, VInt value, unsigned size) override;
  void reset() override;

  enum Parameters { BUFFER_SIZE };

  bool interruptPending() const override;

protected:
  void parameterChanged(unsigned ID) override;

private slots:
  void onKeyTyped(QChar c);

private:
  enum Offsets : AInt {
    DATA_REG  = 0x0
  };

  void initRegDescs();


  std::deque<uint8_t> m_buffer;        // buffer
  std::vector<RegDesc> m_regDescs;
};

} // namespace Ripes

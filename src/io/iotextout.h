#pragma once

#include "iobase.h"
#include <deque>
#include <QPlainTextEdit>
#include <QVBoxLayout>

namespace Ripes {

/**
 * @brief Monitor peripheral: escribe caracteres en un buffer y los muestra
 *        en una caja de texto.
 */
class IOTextOut : public IOBase {
  Q_OBJECT
public:
  explicit IOTextOut(QWidget* parent);
  ~IOTextOut() { unregister(); }

  // Interface IOBase
  QString baseName() const override { return "Monitor"; }
  QString description() const override {
    return "Monitor: muestra caracteres escritos por el procesador en un buffer y los despliega.";
  }
  unsigned byteSize() const override { return 4; }  // registro de 4 bytes (solo byte bajo usado)
  const std::vector<RegDesc>& registers() const override { return m_regDescs; }
  bool supportsInterrupts() const override { return true; }

  VInt ioRead(AInt /*offset*/, unsigned /*size*/) override { return 0; }
  VInt ioReadConst(AInt /*offset*/, unsigned /*size*/) override { return 0; }
  void ioWrite(AInt offset, VInt value, unsigned size) override;
  void reset() override;

  bool interruptPending() const override;

protected:
  void paintEvent(QPaintEvent* event) override;
  void parameterChanged(unsigned ID) override;

private:
  enum Parameters { BUFFER_SIZE };
  enum Offsets { DATA_REG = 0x0 };
  void initRegDescs();

  std::deque<uint8_t>          m_buffer;
  std::vector<RegDesc>         m_regDescs;
  QPlainTextEdit*              m_textEdit = nullptr;
};

} // namespace Ripes
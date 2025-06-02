// iomonitor.cpp
#include "iotextout.h"
#include "ioregistry.h"
#include <QPaintEvent>

namespace Ripes {

IOTextOut::IOTextOut(QWidget* parent)
    : IOBase(IOType::TEXT_OUT, parent) {
  // Parameter to resize the buffer
  m_parameters[BUFFER_SIZE] = IOParam(BUFFER_SIZE, "Buffer size", 1024, true, 1, 10000);

  initRegDescs();

  // GUI
  m_textEdit = new QPlainTextEdit(this);
  m_textEdit->setReadOnly(true);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(m_textEdit);
  setLayout(layout);

  reset();
}

void IOTextOut::initRegDescs() {
  // A single 8-bit write register at offset 0
  m_regDescs = {{ "DATA", RegDesc::RW::W, 8, DATA_REG, false }};
  emit regMapChanged();
}

void IOTextOut::reset() {
  m_buffer.clear();
  if (m_textEdit) m_textEdit->clear();
  emit scheduleUpdate();
}

void IOTextOut::ioWrite(AInt offset, VInt value, unsigned /*size*/) {
  if (offset == DATA_REG) {
    uint8_t c = static_cast<uint8_t>(value & 0xFF);
    // If the buffer is full, discard the oldest entry
    if (m_buffer.size() >= m_parameters.at(BUFFER_SIZE).value.toUInt())
      m_buffer.pop_front();
    m_buffer.push_back(c);
    emit scheduleUpdate();
  }
}

bool IOTextOut::interruptPending() const {
  // Request IRQ whenever the buffer is empty
  return m_buffer.empty();
}

void IOTextOut::parameterChanged(unsigned ID) {
  if (ID == BUFFER_SIZE) {
    m_buffer.clear();
    if (m_textEdit) m_textEdit->clear();
  }
}

void IOTextOut::paintEvent(QPaintEvent* /*event*/) {
  // Dump the buffer to the text box in arrival order
  while (!m_buffer.empty()) {
    uint8_t c = m_buffer.front();
    m_buffer.pop_front();
    if (m_textEdit)
      m_textEdit->insertPlainText(QString::fromLatin1(reinterpret_cast<char*>(&c), 1));
  }
}

} // namespace Ripes

#include "iokeyboard.h"

#include "ioregistry.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <iomanip>

namespace Ripes {

IOKeyboard::IOKeyboard(QWidget *parent)
    : IOBase(IOType::KEYBOARD, parent) {
  m_parameters[BUFFER_SIZE] = IOParam(
    BUFFER_SIZE,
    "Buffer size",
    1024,
    true,
    1,
    9999
  );
  // GUI
  auto* edit = new QLineEdit(this);
  edit->setPlaceholderText("Escribe aquí...");
  edit->setMaxLength(m_parameters.at(BUFFER_SIZE).value.toUInt());
  QHBoxLayout* l = new QHBoxLayout(this);
  l->addWidget(edit);
  setLayout(l);

  connect(edit, &QLineEdit::textChanged, [this, edit]() {
    const auto& txt = edit->text();
    if (!txt.isEmpty()) {
      QChar c = txt.back();
      edit->clear();             // vaciamos la caja
      onKeyTyped(c);             // lo procesamos
    }
  });

  initRegDescs();
  reset();
}

void IOKeyboard::initRegDescs() {
  m_regDescs = {
    { "DATA",  RegDesc::RW::R, 8, DATA_REG,  true }
  };
  emit regMapChanged();
}

void IOKeyboard::reset() {
  m_buffer.clear();
  emit scheduleUpdate();
}

void IOKeyboard::onKeyTyped(QChar c) {
  // Oldest character gets deleted if there is no more space in the buffer
  if (m_buffer.size() == m_parameters.at(BUFFER_SIZE).value.toUInt()) m_buffer.pop_front();
  m_buffer.push_back(static_cast<uint8_t>(c.toLatin1()));
  emit scheduleUpdate();
}


VInt IOKeyboard::ioRead(AInt offset, unsigned size) {
  switch (offset) {
  case DATA_REG: {
    uint8_t v = 0;
    if (!m_buffer.empty()) {
      v = m_buffer.front();
      m_buffer.pop_front();
    }
    return v;
  }

  default:
    return 0;
  }
}

// Interface access
VInt IOKeyboard::ioReadConst(AInt offset, unsigned size) {
  switch (offset) {
  case DATA_REG: {
    uint8_t v = 0;
    if (!m_buffer.empty()) {
      v = m_buffer.front();  // Character doesn't pop
    }

    return v;
  }

  default:
    return 0;
  }
}


void IOKeyboard::ioWrite(AInt, VInt, unsigned) {
  // Register is non-writable
}

void IOKeyboard::parameterChanged(unsigned ID) {
  if (ID == BUFFER_SIZE) {
    m_buffer.clear();
  }
}

bool IOKeyboard::interruptPending() const {
  // we request an interrupt if there is a character in the buffer
  return !m_buffer.empty();
}

} // namespace Ripes

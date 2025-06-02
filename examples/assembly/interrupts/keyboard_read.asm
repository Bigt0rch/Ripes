.global _start
.text

# ---------------------------------------------------------
# Etiqueta de entrada y definiciones
# ---------------------------------------------------------
_start:
    # 1 Definiciones de base y offsets PLIC / teclado
    .equ PLIC_BASE,      0xF0000000
    .equ PRIO_BASE,      PLIC_BASE
    .equ ENAB_BASE,      PLIC_BASE+0x002000
    .equ THRESH_REG,     PLIC_BASE+0x200000
    .equ CLAIM_REG,      PLIC_BASE+0x200004

    .equ KEYBOARD_ID,    1
    .equ KEYBOARD_BASE,  0xF0200008
    .equ KEY_MASK,       2      # (1<<1)

# ---------------------------------------------------------
# 2 Inicialización del PLIC
# ---------------------------------------------------------
    # 2.1 priority[1] = 1
    li   t0, PRIO_BASE
    li   t1, KEYBOARD_ID
    slli t1, t1, 2
    add  t0, t0, t1
    li   t1, 1
    sw   t1, 0(t0)

    # 2.2 enable[0] |= (1<<1)
    li   t0, ENAB_BASE
    li   t1, KEY_MASK
    sw   t1, 0(t0)

    # 2.3 threshold = 0
    li   t0, THRESH_REG
    sw   zero, 0(t0)

# ---------------------------------------------------------
# 3 Habilitar interrupciones en Modo Máquina
# ---------------------------------------------------------
    # 3.1 mstatus.MIE = 1
    csrrs t0, mstatus, x0
    li    t1, 1
    slli  t1, t1, 3
    or    t0, t0, t1
    csrrw x0, mstatus, t0

    # 3.2 mie.MEIE = 1
    csrrs t0, mie, x0
    li    t1, 1
    slli  t1, t1, 11
    or    t0, t0, t1
    csrrw x0, mie, t0

# ---------------------------------------------------------
# 4 Instalar vector de interrupción en mtvec y main loop
# ---------------------------------------------------------
    # Carga la dirección del handler y la escribe en mtvec
    la    t0, irq_handler
    csrrw x0, mtvec, t0

main_loop:
    j main_loop             # Bucle infinito, la CPU entrará en excepción cuando llegue irq

# ---------------------------------------------------------
# 5 Handler de interrupción
# ---------------------------------------------------------
irq_handler:
    # Claim: leer ID de la interrupción activa
    li  t0, CLAIM_REG
    lw   t1, 0(t0)

    # Si es la del teclado (ID=1), lee el dato y guarda en s0
    li   t2, KEYBOARD_ID
    bne  t1, t2, complete

    li   t0, KEYBOARD_BASE
    lbu  t3, 0(t0)
    mv   s0, t3

complete:
    # Complete: rearmar la interrupción
    li   t0, CLAIM_REG
    sw   t1, 0(t0)

    mret                    # Volver de la excepción

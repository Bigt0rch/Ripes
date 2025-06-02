    .global _start
    .text

_start:
    # ---------------------------------------------------------
    # Definiciones para PLIC y Monitor
    # ---------------------------------------------------------
    .equ PLIC_BASE,     0xF0000000
    .equ ENAB_BASE,     PLIC_BASE+0x002000
    .equ THRESH_REG,    PLIC_BASE+0x200000
    .equ CLAIM_REG,     PLIC_BASE+0x200004

    .equ MONITOR_ID,    2
    .equ MONITOR_ADDR,  0xF0200010
    .equ MONITOR_MASK,  4       # (1<<2)

    # ---------------------------------------------------------
    # 1 Inicialización del PLIC para Monitor IRQ
    # ---------------------------------------------------------
    # 1.1 priority[2] = 1
    li   t0, PLIC_BASE
    li   t1, MONITOR_ID
    slli t1, t1, 2
    add  t0, t0, t1
    li   t1, 1
    sw   t1, 0(t0)

    # 1.2 enable = (1<<2)
    li   t0, ENAB_BASE
    li   t1, MONITOR_MASK
    sw   t1, 0(t0)

    # 1.3 threshold = 0
    li   t0, THRESH_REG
    sw   zero, 0(t0)

    # ---------------------------------------------------------
    # 2 Instalar vector de interrupción y bucle principal
    # ---------------------------------------------------------
    la    t0, irq_handler
    csrrw x0, mtvec, t0
    
    # ---------------------------------------------------------
    # 3 Habilitar interrupciones en Modo Máquina
    # ---------------------------------------------------------
    # mstatus.MIE = 1
    csrrs t0, mstatus, x0
    li    t1, 1
    slli  t1, t1, 3
    or    t0, t0, t1
    csrrw x0, mstatus, t0

    # mie.MEIE = 1
    csrrs t0, mie, x0
    li    t1, 1
    slli  t1, t1, 11
    or    t0, t0, t1
    csrrw x0, mie, t0

main_loop:
    j     main_loop

    # ---------------------------------------------------------
    # 4 Handler de interrupción
    # ---------------------------------------------------------
irq_handler:
    # Claim: leer ID de la IRQ activa
    li   t0, CLAIM_REG
    lw   t1, 0(t0)           # t1 = ID de la interrupción

    # Si es la del monitor (ID = 2), escribir 'A'
    li   t2, MONITOR_ID
    bne  t1, t2, complete

    la   t0, MONITOR_ADDR
    li   t3, 65              # ASCII 'A'
    sb   t3, 0(t0)

complete:
    # Complete: rearmar la IRQ
    li   t0, CLAIM_REG
    sw   t1, 0(t0)

    mret

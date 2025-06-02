    .global _start
    .text

_start:
    .equ PLIC_BASE,      0xF0000000
    .equ ENAB_BASE,      PLIC_BASE+0x002000
    .equ THRESH_REG,     PLIC_BASE+0x200000
    .equ CLAIM_REG,      PLIC_BASE+0x200004

    .equ KEYBOARD_ID,    1
    .equ KEYBOARD_BASE,  0xF0200008
    .equ KEY_MASK,       2      # 1<<1

    .equ MONITOR_ID,     2
    .equ MONITOR_ADDR,   0xF020000c
    .equ MON_MASK,       4      # 1<<2

    # ---------------------------------------------------------
    # 1 Inicialización del PLIC
    # ---------------------------------------------------------
    # Dar prioridad 1 al teclado (ID=1)
    li   t0, PLIC_BASE
    li   t1, KEYBOARD_ID
    slli t1, t1, 2
    add  t0, t0, t1
    li   t1, 1
    sw   t1, 0(t0)

    # Dar prioridad 0 al monitor (ID=2), deshabilitando sus IRQ
    li   t0, PLIC_BASE
    li   t1, MONITOR_ID
    slli t1, t1, 2
    add  t0, t0, t1
    sw   zero, 0(t0)

    # Enable: solo teclado
    li   t0, ENAB_BASE
    li   t1, KEY_MASK
    sw   t1, 0(t0)

    # Umbral = 0
    li   t0, THRESH_REG
    sw   zero, 0(t0)

    # ---------------------------------------------------------
    # 2 Habilitar interrupciones en modo máquina
    # ---------------------------------------------------------
    csrrs t0, mstatus, x0
    li    t1, 1
    slli  t1, t1, 3
    or    t0, t0, t1
    csrrw x0, mstatus, t0

    csrrs t0, mie, x0
    li    t1, 1
    slli  t1, t1, 11
    or    t0, t0, t1
    csrrw x0, mie, t0

    # ---------------------------------------------------------
    # 3 Vector de interrupción y bucle principal
    # ---------------------------------------------------------
    la    t0, irq_handler
    csrrw x0, mtvec, t0

main_loop:
    j     main_loop

    # ---------------------------------------------------------
    # 4 Handler de interrupción
    # ---------------------------------------------------------
irq_handler:
    # Claim: leer ID activo
    li   t0, CLAIM_REG
    lw   t1, 0(t0)          # t1 = ID de IRQ

    # Si es teclado (1), leer y escribir en monitor
    li   t2, KEYBOARD_ID
    bne  t1, t2, done

    # Leer byte del teclado usando registro base
    la   t0, KEYBOARD_BASE
    lbu  t3, 0(t0)

    # Escribir en el monitor usando registro base
    la   t1, MONITOR_ADDR
    sb   t3, 0(t1)

done:
    # Rearmar IRQ
    li   t0, CLAIM_REG
    sw   t1, 0(t0)

    mret

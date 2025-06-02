.global __start
.text
__start:
la t0, handler # escribir direccion del hanlder en t0
csrrw zero, mtvec, t0 # escribimos la direccion del handler en mtvec
csrrwi zero, mstatus, 1 # activamos las excepciones
lw zero, 1 #acceso ilegal a memoria
addi t1, x0, 1 
jal x0, exit

handler:
csrrw t0, mepc, zero # escribimos el pc que provocó la excepcion en t0
addi t0, t0, 4 # Adelantamos el PC una instruccion
csrrw zero, mepc, t0 # Excribimos el pc de nuevo en mepc
mret # finalizamos el handler, escribiendo el mepc en el pc

exit:
li a7, 93            # Código de la syscall para 'exit' (93 en RISC-V)
li a0, 0             # Código de salida (0 para éxito)
ecall                # Terminar el programa

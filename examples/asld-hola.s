# asld-hola.s - Prueba de 'as' y 'ld' propios de MiOS (Fase 7).
#
# Se ensambla con el as de MiOS y se enlaza con su ld, sin gcc de por medio:
#
#   as /usr/src/asld-hola.s -o /tmp/hola.o
#   ld -o /tmp/hola --dynamic-linker=/lib/ld-mios.so.1 \
#      /usr/lib/crt1.o /tmp/hola.o -L/lib -lmios
#   /tmp/hola
#
# printf viene de libmios.so.6 (por la PLT), y crt1.o aporta el _start que
# llama a main() y sale con su valor de retorno.

    .text
    .globl  main
    .type   main, @function
main:
    leaq    msg(%rip), %rdi
    xorl    %eax, %eax
    call    printf@PLT
    xorl    %eax, %eax
    ret
    .size   main, .-main

    .section .rodata
msg:
    .string "as y ld de MiOS: hola\n"

/*
 * crt1.c - arranque de los programas de MiOS.
 *
 * El cargador (ld-mios.so.1) salta aqui con la pila que dejo el kernel:
 * en (%rsp) esta argc, luego argv[] y despues envp[], todo terminado en NULL.
 *
 * Orden: se guarda argc/argv/envp en registros que sobreviven a las llamadas
 * (rbx, r12, r13), se le pasa envp a libmios (para getenv/setenv), se llama a
 * main() y se termina con exit(), que vacia los buffers antes de salir.
 *
 * Es la version "de programa normal" de lo que hace src/core/main.c para init:
 * init nunca vuelve (es el PID 1); un programa cualquiera si.
 */

extern int main(int argc, char **argv, char **envp);
extern void exit(int status) __attribute__((noreturn));
extern void __mios_set_environ(char **envp);

__attribute__((naked, noreturn)) void _start(void) {
    __asm__ volatile(
        "xor %rbp, %rbp\n"              /* fin de pila, marca al depurador  */
        "mov (%rsp), %rbx\n"            /* argc                            */
        "lea 8(%rsp), %r12\n"           /* argv                            */
        "lea 16(%rsp,%rbx,8), %r13\n"   /* envp = argv + argc + 1          */
        "and $-16, %rsp\n"              /* pila alineada a 16              */
        "mov %r13, %rdi\n"
        "call __mios_set_environ\n"     /* getenv/environ (libmios)        */
        "mov %rbx, %rdi\n"
        "mov %r12, %rsi\n"
        "mov %r13, %rdx\n"
        "call main\n"
        "mov %eax, %edi\n"              /* valor de retorno de main        */
        "call exit\n"
        "ud2\n");
}

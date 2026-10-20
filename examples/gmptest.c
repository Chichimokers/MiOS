/* Prueba de GMP, MPFR y MPC sobre libmios (Fase 8).
 *
 * Las tres bibliotecas aritmeticas que necesita GCC.  Este programa calcula
 * cosas con cada una y comprueba los resultados, de forma que se pueda ver de
 * un vistazo si estan bien enlazadas, si el cargador las carga y si la libc
 * que piden funciona.
 *
 * Se puede ejecutar en el VPS (con LD_LIBRARY_PATH al prefijo), dentro del
 * chroot del sysroot o dentro de MiOS.
 */
#include <stdio.h>
#include <string.h>

#include <gmp.h>
#include <mpfr.h>
#include <mpc.h>

static int fallos = 0;

static void check(const char *que, const char *esperado, const char *obtenido) {
    if (strcmp(esperado, obtenido) == 0) {
        printf("  ok    %-22s = %s\n", que, obtenido);
    } else {
        printf("  FALLO %-22s: esperado '%s', obtenido '%s'\n",
               que, esperado, obtenido);
        fallos++;
    }
}

int main(void) {
    char buf[256];

    printf("MiOS / Fase 8: GMP %s, MPFR %s, MPC %s\n",
           gmp_version, mpfr_get_version(), mpc_get_version());

    /* GMP: 100! (158 cifras, valor conocido) */
    mpz_t f;
    mpz_init_set_ui(f, 1);
    for (unsigned i = 2; i <= 100; i++)
        mpz_mul_ui(f, f, i);
    mpz_get_str(buf, 10, f);
    check("100! (GMP)",
          "9332621544394415268169923885626670049071596826438162146859296389521"
          "7599993229915608941463976156518286253697920827223758251185210916864"
          "000000000000000000000000",
          buf);
    mpz_clear(f);

    /* MPFR: pi y raiz de 2 (con 200 bits, para que los 20 decimales que se
     * comparan sean los de verdad y no los redondeados a 64 bits) */
    mpfr_t p;
    mpfr_init2(p, 200);
    mpfr_const_pi(p, MPFR_RNDN);
    mpfr_snprintf(buf, sizeof buf, "%.20Rf", p);
    check("pi (MPFR)", "3.14159265358979323846", buf);

    mpfr_sqrt_ui(p, 2, MPFR_RNDN);
    mpfr_snprintf(buf, sizeof buf, "%.20Rf", p);
    check("sqrt(2) (MPFR)", "1.41421356237309504880", buf);

    mpfr_exp(p, p, MPFR_RNDN);              /* e^sqrt(2) = 4.1132503787... */
    mpfr_snprintf(buf, sizeof buf, "%.10Rf", p);
    check("exp(sqrt 2) (MPFR)", "4.1132503788", buf);
    mpfr_clear(p);

    /* MPC: (1+2i) * (3+4i) = -5+10i */
    mpc_t a, b, c;
    mpc_init2(a, 64);
    mpc_init2(b, 64);
    mpc_init2(c, 64);
    mpc_set_ui_ui(a, 1, 2, MPC_RNDNN);
    mpc_set_ui_ui(b, 3, 4, MPC_RNDNN);
    mpc_mul(c, a, b, MPC_RNDNN);
    mpfr_snprintf(buf, sizeof buf, "%.0Rf%+.0Rfi",
                  mpc_realref(c), mpc_imagref(c));
    check("(1+2i)*(3+4i) (MPC)", "-5+10i", buf);
    mpc_clear(a);
    mpc_clear(b);
    mpc_clear(c);

    printf("Fase 8: %d fallo(s)\n", fallos);
    return fallos;
}

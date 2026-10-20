    /* -----------------------------------------------------------------
     * MiOS: informar del resultado de la compilacion.
     *
     * tcc se calla cuando todo va bien, y asi el usuario no sabe si el
     * fichero se ha generado o no; al fallar, en cambio, deja los mensajes
     * de error sueltos sin decir claramente que la compilacion no termino.
     *
     * tools/build-tcc.sh inserta este trozo (tal cual) al final de main()
     * de tcc.c, justo antes de tcc_delete(s), cuando copia las fuentes de
     * tcc al directorio de compilacion: el tcc.c de third_party/ no se toca.
     * Como la copia parcheada es tambien la que se instala en la ISO
     * (/usr/src/tcc/tcc.c), el tcc que MiOS compila a si mismo se comporta
     * igual que /bin/tcc.
     * ----------------------------------------------------------------- */
    if (s->run_test == 0 && s->output_type != TCC_OUTPUT_PREPROCESS) {
        if (ret == 0) {
            fprintf(stderr, "tcc: compilacion correcta -> %s\n",
                    s->outfile ? s->outfile : "(sin fichero)");
        } else {
            fprintf(stderr, "tcc: la compilacion ha fallado\n");
        }
    }

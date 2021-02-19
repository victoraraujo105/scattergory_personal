#include <stdlib.h>
#include <stdio.h>
#include <main.h>
#include <limits.h>
#include <sys/select.h>

/*
 *  - PROPÓSITO:
 * 
 *  Aguarda a modifição do fluxo padrão de entrada (stdin).
 * 
 *  - PARÂMETROS:
 *  
 *  <timeout> aponta para struct <timeval> de atributos
 *  <tv_sec> e <tv_usec>, que, respectivamente,
 *  representam segundos e microssegundos a serem esperados.
 * 
 *  - RETORNO:
 * 
 *  retorna 0, caso o tempo limite seja ultrapassado;
 *
 *         -1, caso <timeout> passado seja inválido ou ocorra algum outro erro
 *  durante a execução de <select()>, especificado pela variável <errno>;
 *
 *          valores positivos, caso <stdin> tenha sido alterado.
 */

int await_input(struct timeval *timeout) {
    fd_set readfds; /* file descriptor de <stdin> */

    FD_ZERO(&readfds);  /* inicializando-o */ 
    FD_SET(fileno(stdin), &readfds);    /* associando-o a <stdin> */

    return select(1, &readfds, NULL, NULL, timeout);    /* aguarda modificação da entrada */
}

char *read_line(FILE *f) {
    int i = 0, s = 128, c;
    char *buffer = malloc(s);

    while ((c = fgetc(stdin)) != EOF && c != (int) '\n') {
        
        if (i == s - 1) {
            s += 128;
            buffer = realloc(buffer, s);
        }
        
        buffer[i] = c;
        i++;
    }

    buffer[i] = 0;

    return buffer;
}

int main(int argc, char *argv[]) {
    game_data data;

    struct timeval timeout;

    while (1)
    {
        timeout.tv_sec = 2;

        if (await_input(&timeout) == 0) {
            puts("timeout");

        } else {
            puts(read_line(stdin));
        }
    }
    
    return EXIT_SUCCESS;
}
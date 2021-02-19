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

    while ((c = fgetc(f)) != EOF && c != (int) '\n') {
        
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

/*
 *  Tenta receber inteiro em [min, max] dentro do intervalo de tempo estabelecido.
 */

long long get_int(long long min, long long max, struct timeval *timeout) {
        int input_status;
        long long n;

        while ((input_status = await_input(timeout)) > 0) {
            sscanf(read_line(stdin), "%lld", &n);
            if (min <= n && n <= max) return n;
            else {
                fputs("\nInteiro fora do intervalo esperado!\n\nDigite valor ", stdout);
                
                if (max == LLONG_MAX) printf("superior a %lld: ", min);
                else if (min == LLONG_MIN) printf("inferior a %lld: ", max);
                else printf("em [%lld, %lld]: ", min, max);
                fflush(stdout);
            }
        }

        return input_status;
}

int main(int argc, char *argv[]) {
    game_data data;

    fputs("Insira o número de jogadores (entre 2 e 10): ", stdout);
    fflush(stdout);

    data.number_of_players = (int) get_int(2, 10, NULL);

    printf("Número de jogadores: %d\n", data.number_of_players);

    return EXIT_SUCCESS;
}
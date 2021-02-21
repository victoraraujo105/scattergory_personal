#include <stdlib.h>
#include <stdio.h>
#include <main.h>
#include <limits.h>
#include <sys/select.h>
#include <time.h>
#include <locale.h>
#include <stdarg.h>

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

int await_input(struct timeval *timeout)
{
    fd_set readfds; /* file descriptor de <stdin> */

    FD_ZERO(&readfds);               /* inicializando-o */
    FD_SET(fileno(stdin), &readfds); /* associando-o a <stdin> */

    return select(1, &readfds, NULL, NULL, timeout); /* aguarda modificação da entrada */
}

wchar_t *read_up_to(FILE *f, wint_t terminator)
{ /* returns text section up to <terminator>, not including it, or EOF */
    int i = 0, s = 128;
    wchar_t *buffer = malloc(s * WCHAR_SIZE), *temp_buffer;
    wint_t c;

    if (buffer != NULL)
    {

        while ((c = fgetwc(f)) != WEOF && c != terminator)
        {

            if (i == s - 1)
            {
                s += 128;
                temp_buffer = realloc(buffer, s * WCHAR_SIZE);
                if (temp_buffer == NULL)
                {
                    free(buffer);
                    return NULL;
                }
                buffer = temp_buffer;
            }

            buffer[i] = c;
            i++;
        }

        buffer[i] = 0;
    }

    return buffer;
}

wchar_t *read_line(FILE *f)
{
    return read_up_to(f, L'\n');
}

/*
 *  Tenta receber inteiro em [min, max] dentro do intervalo de tempo estabelecido.
 */

long long get_int(long long min, long long max, struct timeval *timeout)
{
    int input_status, conversion_status;
    long long n;
    wchar_t *s;

    while ((input_status = await_input(timeout)) > 0)
    {
        s = read_line(stdin);

        if (s == NULL)
        {
            timeout->tv_sec = 0;
            timeout->tv_usec = 0;

            return -1;
        }

        conversion_status = swscanf(s, L"%lld", &n);

        free(s);

        if (conversion_status == 1 && min <= n && n <= max)
            return n;
        else
        {
            fputws(L"\n\tEntrada inválida ou fora do intervalo esperado!\n\nDigite inteiro", stdout);

            if (min == LLONG_MIN && max == LLONG_MAX)
                fputws(L": ", stdout);
            else if (max == LLONG_MAX)
                wprintf(L" superior a %lld: ", min);
            else if (min == LLONG_MIN)
                wprintf(L" inferior a %lld: ", max);
            else
                wprintf(L" em [%lld, %lld]: ", min, max);
            fflush(stdout);
        }
    }

    return input_status; /* if timeout expires, timeout fields and return value are all 0;
        whenever error occurrs, timeout fields are set to 0 and -1 is returned,
        the reason should be found by checking the corresponding <errno> value.
    */
}

wchar_t *trim_wstring(wchar_t *s)
{
    int i = 0; /* current <s> index */
    int j = 0; /* current <trimmed> index */
    int n = 128;
    wchar_t *trimmed = malloc(n * WCHAR_SIZE), *temp_trimmed; /* trimmed string */

    if (trimmed != NULL)
    {
        /* skip left-hand spaces */
        while (s[i] == L' ')
            i++;

        while (s[i] != 0)
        {
            if (s[i] != L' ' || s[i - 1] != L' ')
            {
                if (j == n - 1)
                {
                    n += 128;
                    temp_trimmed = realloc(trimmed, n * WCHAR_SIZE);

                    if (temp_trimmed == NULL)
                    {
                        free(trimmed);
                        return NULL;
                    }

                    trimmed = temp_trimmed;
                }
                trimmed[j] = s[i];
                j++;
            }

            i++;
        }

        if (j > 0 && trimmed[j - 1] == L' ')
            j--;

        trimmed[j] = 0;

        temp_trimmed = realloc(trimmed, (j + 1) * WCHAR_SIZE);

        if (temp_trimmed != NULL)
            trimmed = temp_trimmed;
    }

    return trimmed;
}

int wstr_size(wchar_t *s)
{
    int i = 0;

    while (s[i] != 0)
        i++;

    return i;
}

wchar_t *get_answer(wchar_t *prompt, unsigned long long min_size, unsigned long long max_size, struct timeval *timeout)
{ /* aks for input until gets answer within size constraint or timeout is elapsed;
    for undefined lim, pass <ULLONG_MAX> from <limits.h> as second argument  */
    wchar_t *raw_anwser, *answer;
    int input_status, size_answer, first_loop = 1;

    do
    {
        if (!first_loop)
        {

            free(answer);

            if (size_answer > max_size)
                wprintf(L"\n\tEntrada não deve exceder %d caracteres!\n", max_size);
            else if (min_size == 1)
                putws(L"\n\tEntrada vazia!");
            else
                wprintf(L"\n\tInsira ao menos %d caracteres!\n", min_size);

            // putws("\n\tOBS: espaços extremos são ignorados e os internos contíguos contados única vez.")
        }

        fputws(prompt, stdout);
        fflush(stdout);

        input_status = await_input(timeout);

        if (input_status <= 0)
            return NULL; /* time expired (input_status == 0) or <select()> failed (input_status == -1) (check <errno>)*/

        raw_anwser = read_line(stdin);

        if (raw_anwser == NULL) /* unable to allocate memory to store line (errno == ENOMEM) */
            return NULL;

        answer = trim_wstring(raw_anwser);

        free(raw_anwser);

        size_answer = wstr_size(answer);

        first_loop = 0;
    } while (size_answer < min_size || size_answer > max_size);

    return answer;
}

wchar_t *fwstring(const wchar_t *format, ...)
{
    wchar_t *mem_buffer, *s = NULL;
    size_t mem_size;
    FILE *mem_stream = open_wmemstream(&mem_buffer, &mem_size);

    va_list ap;

    int operation_status;

    if (mem_stream == NULL)
        return NULL;

    va_start(ap, format);

    operation_status = vfwprintf(mem_stream, format, ap);

    va_end(ap);

    if (operation_status != -1)
    {
        s = read_up_to(mem_stream, WEOF);
    }

    fclose(mem_stream);
    free(mem_buffer);

    return s;
}

int get_names(game_data *data)
{
    int i;
    wchar_t *name;

    data->player_name = malloc(sizeof(wchar_t *) * data->number_of_players);

    if (data->player_name == NULL)
        return -1;

    for (i = 0; i < data->number_of_players; i++)
    {
        name = get_answer(fwstring(L"\nNome do jogador %02d: ", i + 1), 1, data->name_size, NULL);

        if (name == NULL)
            return -1;

        putwchar(L'\n');
        data->player_name[i] = name;
    }

    return 0; /* job done */
}

void swap_int(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

int rand_int(int lower_bound, int upper_bound)
{ /* returns random integer in [lower_bound, upper_bound) */

    return rand() % (upper_bound - lower_bound) + lower_bound;
}

int *index_permutation(int n)
{ /* Fisher-Yates shuffle */
    int i, r, *indices = malloc(sizeof(int) * n);

    for (i = 0; i < n; i++)
        indices[i] = i;

    for (i = 0; i < n - 1; i++)
    {
        r = rand_int(i, n);
        swap_int(&indices[i], &indices[r]);
    }

    return indices;
}

void show_players(game_data *data)
{
    int i;
    wchar_t **player = data->player_name;
    int *sequence = data->players_sequence;
    for (i = 0; i < data->number_of_players; i++)
    {
        wprintf(L"\t%2d. %S\n", i + 1, player[sequence[i]]);
    }
}

int main(int argc, char *argv[])
{
    const int name_size = 12;
    const int number_of_letters = 23;
    const wchar_t *const letters = L"ABCDEFGHIJLMNOPQRSTUVXZ";
    const int rounds = 4;

    setlocale(LC_ALL, "");
    srand(time(NULL));

    game_data data = {name_size, number_of_letters, letters, rounds};

    fputws(L"Insira o número de jogadores (entre 2 e 10): ", stdout);
    fflush(stdout);

    data.number_of_players = (int)get_int(2, 10, NULL);

    wprintf(L"Número de jogadores: %d\n", data.number_of_players);

    /*
    fputs("\n\nDigite um nome: ", stdout);

    fflush(stdout);

    char *name = read_line(stdin);
    
    char *trimmed_name = trim_wstring(name);

    printf("Nome formatado: %s.\n", trimmed_name);

    free(name);

    free(trimmed_name);
    */

    get_names(&data);

    for (int i = 0; i < data.number_of_players; i++)
    {
        wprintf(L"Jogador %02d: %S\n", i + 1, data.player_name[i]);
    }

    data.letters_sequence = index_permutation(data.number_of_letters);

    // int *A, n = 100;

    for (data.curr_round = 0; data.curr_round < data.rounds; data.curr_round++)
    {
        wprintf(L"Letra da rodada: %c\n", data.letters[data.letters_sequence[data.curr_round]]);

        data.players_sequence = index_permutation(data.number_of_players);

        //    A = index_permutation(n);

        //    for (int i = 0; i < n; i++) {
        //        if (i == n - 1) wprintf(L"%d.\n", A[i]);
        //        else wprintf(L"%d, ", A[i]);
        //    }

        // wprintf(L"%d", rand_int(0, 100));

        putws(L"Ordem da rodada:");
        show_players(&data);

        free(data.players_sequence);
    }

    return EXIT_SUCCESS;
}
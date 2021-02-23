#include <stdlib.h>
#include <stdio.h>
#include <main.h>
#include <limits.h>
#include <sys/select.h>
#include <time.h>
#include <locale.h>
#include <stdarg.h>
#include <errno.h>
#include <wctype.h>

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

int await_input(time_data *timeout)
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

long long get_int(long long min, long long max, time_data *timeout)
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

wchar_t *trim_wstring(wchar_t const *s)
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

int wstr_size(wchar_t const *s)
{
    int i = 0;

    while (s[i] != 0)
        i++;

    return i;
}

int wstr_find(wchar_t const *s, wchar_t c)
{
    int i = 0;

    while (s[i] != 0 && s[i] != c)
        i++;

    if (s[i] == 0)
        i = -1;

    return i;
}

int validate_answer(wchar_t *answer, unsigned long long min_size, unsigned long long max_size)
{
    int size_answer = wstr_size(answer);

    if (size_answer < min_size || size_answer > max_size)
    {
        free(answer);

        if (size_answer > max_size)
            wprintf(L"\n\tEntrada não deve exceder %d caracteres!\n\n", max_size);
        else if (min_size == 1)
            putws(L"\n\tEntrada vazia!\n");
        else
            wprintf(L"\n\tInsira ao menos %d caracteres!\n\n", min_size);

        // putws("\n\tOBS: espaços extremos são ignorados e os internos contíguos contados única vez.");

        return 0;
    }
    else
        return 1;
}

wchar_t *fget_input(unsigned long long min_size, unsigned long long max_size, time_data *timeout, wchar_t *format, ...)
{ /* aks for input until gets answer within size constraint or timeout is elapsed;
    for undefined lim, pass <ULLONG_MAX> from <limits.h> as second argument  */
    wchar_t *raw_anwser, *answer, *prompt;
    int input_status;

    va_list ap;

    do
    {

        va_start(ap, format);

        prompt = vfwstring(format, ap);

        if (prompt == NULL)
            return NULL;

        fputws(prompt, stdout);

        fflush(stdout);

        free(prompt);

        input_status = await_input(timeout);

        if (input_status <= 0)
            return NULL; /* time expired (input_status == 0) or <select()> failed (input_status == -1) (check <errno>)*/

        raw_anwser = read_line(stdin);

        if (raw_anwser == NULL) /* unable to allocate memory to store line (errno == ENOMEM) */
            return NULL;

        answer = trim_wstring(raw_anwser);

        free(raw_anwser);

    } while (!validate_answer(answer, min_size, max_size));

    va_end(ap);

    return answer;
}

wchar_t *get_input(wchar_t *prompt, unsigned long long min_size, unsigned long long max_size, time_data *timeout, int flush)
{
    /* aks for input until gets answer within size constraint or timeout is elapsed;
    for undefined lim, pass <ULLONG_MAX> from <limits.h> as second argument  */
    wchar_t *raw_anwser, *answer;
    int input_status;

    do
    {
        if (timeout == NULL)
            fputws(prompt, stdout);
        else
            wprintf(prompt, time_left(*timeout));

        fflush(stdout);

        input_status = await_input(timeout);

        if (input_status <= 0)
            return NULL; /* time expired (input_status == 0) or <select()> failed (input_status == -1) (check <errno>)*/

        raw_anwser = read_line(stdin);

        if (raw_anwser == NULL) /* unable to allocate memory to store line (errno == ENOMEM) */
            return NULL;

        answer = trim_wstring(raw_anwser);

        free(raw_anwser);

        if (flush)
            clear();

    } while (!validate_answer(answer, min_size, max_size));

    return answer;
}

wchar_t *vfwstring(const wchar_t *format, va_list ap)
{
    wchar_t *mem_buffer, *s = NULL;
    size_t mem_size;
    FILE *mem_stream = open_wmemstream(&mem_buffer, &mem_size);

    int operation_status;

    if (mem_stream == NULL)
        return NULL;

    // va_start(ap, format);

    operation_status = vfwprintf(mem_stream, format, ap);

    if (operation_status != -1)
    {
        s = read_up_to(mem_stream, WEOF);
    }

    fclose(mem_stream);
    free(mem_buffer);

    return s;
}

wchar_t *fwstring(const wchar_t *format, ...)
{
    wchar_t *s = NULL;

    va_list ap;

    va_start(ap, format);

    s = vfwstring(format, ap);

    va_end(ap);

    return s;
}

int get_names(game_data *data)
{
    int i;
    wchar_t *name, *prompt;

    data->player_name = malloc(sizeof(wchar_t *) * data->number_of_players);

    if (data->player_name == NULL)
        return -1;

    for (i = 0; i < data->number_of_players; i++)
    {
        prompt = fwstring(L"\nNome do jogador %02d: ", i + 1);
        name = get_input(prompt, 1, data->name_size, NULL, 0);
        free(prompt);

        if (name == NULL)
            return -1;

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

int *ascending_sequence(int n) {
    int *A = calloc(n, sizeof(int));

    for (int i = 0; i < n; i++) A[i] = i;

    return A;
}

int *index_permutation(int n)
{ /* Fisher-Yates shuffle */
    int i, r, *indices;

    if (n <= 0)
        return NULL;

    indices = ascending_sequence(n);

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

void set_time(time_data *td, double sec)
{
    td->tv_sec = (int)sec;
    td->tv_usec = (int)round((int)(sec * 1E-6) % ((int)1E6));
}

double time_left(time_data td)
{
    double s;

    s = td.tv_sec + td.tv_usec * 1E-6;

    return s;
}

double player_total_time(game_data *data)
{
    double min_time = data->min_time;
    double time_decrement = data->time_decrement;
    int players = data->number_of_players;
    int turn = data->curr_turn;

    return min_time + (players - (turn + 1)) * time_decrement;
}
// double get_player_time(&data, int player) {

// }

// int set_player_time(game_data *data, int player, int turn) {

// }

wchar_t *get_answer(game_data *data)
{
    wchar_t *answer;
    time_data *timeout = &data->curr_time_left;

    int cat_id = data->categories_sequence[data->curr_round];

    wchar_t *name = data->player_name[data->players_sequence[data->curr_turn]];
    const wchar_t *category = data->categories[cat_id];
    const wchar_t letter = data->letters[data->letters_sequence[data->curr_round]];

    wchar_t *prompt = fwstring(L"%S, você tem %s segundo(s) para inserir palavra na categoria \"%S\" começando com \"%C\": ", name, "%.2lf", category, letter);

    int first_loop = 1;

    do
    {
        if (!first_loop) wprintf(L"\n\tA letra da rodada é \"%C\"!!\n\n", letter);

        answer = get_input(prompt, 1, 30, timeout, 1);

        // wprintf(L"time_left(timeout) == %lf", time_left(timeout));

        if (answer == NULL)
            break; /* if time hasn't expired at this point, error has ocurred; errno should be checked */

        first_loop = 0;

    } while (!starts_with(answer, letter));

    if (cat_id == 0) {
        int first_space = wstr_find(answer, L' ');
        
        if (first_space != -1) {
            wchar_t *trunc_answer = calloc(first_space + 1, WCHAR_SIZE);
            
            trunc_answer[first_space] = L'\0';

            while (first_space-- > 0) trunc_answer[first_space] = answer[first_space];             

            free(answer);

            answer = trunc_answer;
        } 
    }

    wprintf(L"shit\n");

    free(prompt);

    wprintf(L"shit\n");

    return answer;
}

/* to fucking hell with this

wchar_t *answer_countdown(game_data *data)
{
    wchar_t *raw_answer = fwstring(L"%C", L'\0'), *answer;
    int const max_size = data->answer_size;
    double time_left = player_total_time(data);
    time_t dt;

    int cat_id = data->categories_sequence[data->curr_round];

    wchar_t *name = data->player_name[data->players_sequence[data->curr_round]];
    const wchar_t *category = data->categories[cat_id];
    const wchar_t letter = data->letters[data->letters_sequence[data->curr_round]];

    putws(L"shit1");

    wchar_t *template = fwstring(L"%%S%S, você tem %%S segundo(s) para inserir palavra na categoria \"%S\" começando com \"%C\": %%S", name, category, letter), *prompt, *warning;
    int first_loop = 1, line_break;

    putws(L"shit2");

    // raw_answer = malloc(WCHAR_SIZE);
    // *raw_answer = 0;

    putws(L"shit3");

    do
    {
        line_break = -1;

        // free(read_up_to(stdin, WEOF));

        warning = fwstring(L"%C", L'\0');

        putws(L"shit4");

        if (!first_loop)
        {

            free(warning);
            if (answer[0] == letter)
                warning = fwstring(L"\n\tApenas o primeiro nome!!\n\n");
            else
                warning = fwstring(L"\n\tA letra da rodada é \"%C\"!!\n\n", letter);
        }
        else
            first_loop = 0;

        prompt = fwstring(template, warning, L"%.2lf", L"%S");

        free(warning);

        fflush(stdout);

        do
        {
            dt = -clock();

            clear();

            wprintf(L"time_left == %lf\n\n", time_left);
            
            wprintf(prompt, time_left, raw_answer);
            fflush(stdout);

            free(raw_answer);

            wprintf(L"\nbfr read_up_to(stdin, WEOF)\n\n");
            fflush(stdin);
            raw_answer = read_up_to(stdin, WEOF);
            fflush(stdout);

            wprintf(L"\naftr read_up_to(stdin, WEOF)\n\n");

            if (raw_answer == NULL)
                return NULL;

            line_break = wstr_find(raw_answer, L'\n');
            wprintf(L"\nline_break == %d\n", line_break);

            dt += clock();

            time_left -= (((double)dt) / CLOCKS_PER_SEC);

            if (round(time_left * 100) / 100 <= 0)
            {
                set_time(&data->curr_time_left, 0);
                free(raw_answer);
                return NULL;
            }

        } while (line_break == -1);

        raw_answer[line_break] = 0;

        answer = trim_wstring(raw_answer);

        if (answer == NULL)
            return NULL;

        free(raw_answer);

        set_time(&data->curr_time_left, time_left);

    } while (!validate_answer(answer, 1, max_size) || answer[0] != letter || (cat_id == 0 && wstr_find(answer, ' ') != -1));

    free(prompt);

    return answer;
} 
*/

void show_answers(game_data *data)
{
    int i, player;
    wchar_t *name, *answer;

    wprintf(L"Respostas da %dª Rodada:\n\n", data->curr_round + 1);

    for (i = 0; i < data->number_of_players; i++)
    {
        player = data->players_sequence[i];

        name = data->player_name[player];
        answer = data->round_answer[player];

        wprintf(L"\t%12S: %S\n", name, answer);

        free(answer);
    }
}

int starts_with(wchar_t *s, wchar_t l)
{
    const wchar_t alpha[] = L"AEIOUYBCDFGHJKLMNPQRSTVWXZ";
    const wchar_t variants[] = L"AÁÀÂÃEÉÈÊEIÍÌÎIOÓÒÔÕUÚÙÛUYÝ";

    int letter = wstr_find(alpha, towupper(l));

    int i;

    if (letter == -1)
    {
        letter = wstr_find(variants, towupper(l));

        if (letter == -1)
            return -1;

        letter = letter / 5;
    };

    if (letter < 6)
    {
        i = wstr_find(variants, towupper(s[0]));

        return (5 * letter <= i && i < 5 * (letter + 1));
    }
    else
    {
        return (wstr_find(alpha, towupper(s[0])) == letter);
    }
}

int fconcat(FILE *stream, wchar_t *start, wchar_t *end)
{
    if (fputws(start, stream) == -1)
        return -1;
    if (fputws(end, stream) == -1)
        return -1;
    return 1;
}

wchar_t *concat(wchar_t *start, wchar_t *end)
{
    wchar_t *buffer;
    size_t len;
    FILE *mem_stream = open_wmemstream(&buffer, &len);

    int n = fconcat(mem_stream, start, end);

    fclose(mem_stream);

    if (n == -1)
    {
        free(buffer);
        buffer = NULL;
    }

    return buffer;
}

void fcentered(FILE *stream, wchar_t *s, wchar_t placeholder, int field_width)
{
    int i, str_len = wstr_size(s);

    int remaining_length = (field_width - str_len);

    if (remaining_length <= 0)
        remaining_length = 0;

    for (i = 0; i < remaining_length / 2; i++)
        putwc(placeholder, stream);

    for (i = 0; i < str_len; i++)
        putwc(s[i], stream);

    for (i = 0; i < remaining_length - remaining_length / 2; i++)
        putwc(placeholder, stream);
}

wchar_t *centered(wchar_t *s, wchar_t placeholder, int field_width)
{
    wchar_t *buffer;
    size_t len;
    FILE *mem_stream = open_wmemstream(&buffer, &len);

    int str_len = wstr_size(s);

    int remaining_length = (field_width - str_len), i;

    if (remaining_length <= 0)
        remaining_length = 0;

    for (i = 0; i < remaining_length / 2; i++)
        putwc(placeholder, mem_stream);

    for (i = 0; i < str_len; i++)
        putwc(s[i], mem_stream);

    fflush(mem_stream); /* required to <len> stay up to date */

    for (i = len; i < field_width; i++)
        putwc(placeholder, mem_stream);

    fclose(mem_stream);

    return buffer;
}

void frepeat(FILE *stream, wchar_t *r, wchar_t *sep, int n)
{

    for (int i = 0; i < n - 1; i++)
    {
        fputws(r, stream);
        if (sep != NULL) fputws(sep, stream);
    }

    fputws(r, stream);
}

int max_str(wchar_t const * const*S, int n)
{

    int max_len = 0, size;

    for (int i = 0; i < n; i++)
    {
        size = wstr_size(S[i]);

        if (size > max_len)
            max_len = size;
    }

    return max_len;
}

int sum(int *A, int n)
{
    double s = 0;

    for (int i = 0; i < n; i++)
        s += A[i];

    return s;
}

void *init_array(void *A, unsigned len) {
    unsigned char *p = A;

    while (len--) {
        *p++ = 0;
    }

    return A; 
}

void show_scores(game_data *data)
{
    int round = data->curr_round, cat;

    if (round < 0 || round >= data->rounds)
        return;

    // wchar_t h_template = fwstring("Jogador  ")

    // wchar_t *header = fwstring("Categoria ")

    int cat_field_w = max_str(data->categories, data->rounds);
    size_t len_h;
    // wchar_t *nome = centered(L"Nome", L' ', cat_field_w);
    // wchar_t *de = centered(L"de", L' ', cat_field_w);
    // wchar_t *buffer1, *buffer2;
    wchar_t *buffer, *header, sep[] = L" | ", placeholder = L' ';

    FILE *h_stream = open_wmemstream(&header, &len_h);

    frepeat(h_stream, L" ", NULL, data->name_size);

    fputws(sep, h_stream);

    buffer = centered(L"Nome", placeholder, cat_field_w);

    frepeat(h_stream, buffer, sep, round + 1);

    fputws(sep, h_stream);

    frepeat(h_stream, L" ", NULL, cat_field_w);

    putwc(L'\n', h_stream);

    free(buffer);


    // frepeat(h_stream, L" ", NULL, data->name_size);

    fcentered(h_stream, L"Jogador", placeholder, data->name_size);

    fputws(sep, h_stream);

    buffer = centered(L"de", placeholder, cat_field_w);

    frepeat(h_stream, buffer, sep, round + 1);

    fputws(sep, h_stream);

    free(buffer);

    // buffer = centered(L"Total", placeholder, cat_field_w);

    // fputws(buffer, h_stream);

    fcentered(h_stream, L"Total", placeholder, cat_field_w);

    putwc(L'\n', h_stream);

    frepeat(h_stream, L" ", NULL, data->name_size);

    fputws(sep, h_stream);

    // FILE *b_stream = open_wmemstream(&buffer, &len_b);

    // wchar_t *s1 = fwstring(L"%12S%S", L"Nome", L"%S");

    wchar_t *cat_name;

    for (cat = 0; cat < round + 1; cat++)
    {

        cat_name = (wchar_t *) data->categories[data->categories_sequence[cat]];

        // buffer = centered(cat_name, L" ", cat_field_w);

        // fputws(buffer, h_stream);

        // free(buffer);

        fcentered(h_stream, cat_name, placeholder, cat_field_w);

        fputws(sep, h_stream);
    }

    buffer = centered(L"Parcial", placeholder, cat_field_w);

    fputws(buffer, h_stream);

    fputws(L"\n", h_stream);

    free(buffer);

    frepeat(h_stream, L"*", NULL, (round + 2)*wstr_size(sep) + data->name_size + (round + 2)*cat_field_w);

    fputws(L"\n", h_stream);

    wchar_t *name;
    int player, score;

    for (int turn = 0; turn < data->number_of_players; turn++)
    {
        player = data->players_sequence[turn];

        for (int i = 0; i < round + 3; i++)
        {
            if (i == 0)
            {
                name = data->player_name[player];
                fcentered(h_stream, name, placeholder, data->name_size);
            }
            else if (i == round + 2)
            {
                score = sum(data->score[player], data->rounds);
                buffer = fwstring(L"%d", score);
                fcentered(h_stream, buffer, placeholder, cat_field_w);
                free(buffer);
                break;
            }
            else
            {
                score = data->score[player][data->categories_sequence[i - 1]];
                buffer = fwstring(L"%d", score);
                fcentered(h_stream, buffer, placeholder, cat_field_w);
                free(buffer);
            }

            fputws(sep, h_stream);
            
        }
        putwc(L'\n', h_stream);
    }

    fclose(h_stream);

    fputws(header, stdout);

    free(header);
}

void line_breaks(int n) {
    frepeat(stdout, L"\n", NULL, n);
}

int victor(game_data *data) {
    int **score = data->score, rounds = data->rounds;

    int champ = 0, p, champ_score = sum(score[0], rounds), p_score;

    int *timing = data->time_used;

    for (p = 1; p < data->number_of_players; p++) {
        
        p_score = sum(score[p], rounds);
        
        if (p_score > champ_score) {
            champ_score = p_score;
            champ = p;
        } else if (p_score == champ_score) {
            if (timing[p] < timing[champ]) champ = p;
        }

    }

    return champ;

}

int same_str(wchar_t *a, wchar_t *b) {
    int l = wstr_size(a);
    if (l != wstr_size(b)) return 0;

    for (int i = 0; i < l; i++) {
        if (towupper(a[i]) != towupper(b[i])) return 0;
    }

    return 1;
}

int main(int argc, char *argv[])
{
    const int name_size = 12;
    const int number_of_letters = 23;
    const wchar_t *const letters = L"ABCDEFGHIJLMNOPQRSTUVXZ";
    const int rounds = 5;
    const wchar_t *const categories[] = {L"Pessoas", L"Cidades", L"Animais", L"Comidas", L"Profissões"};
    const double min_time = 8;
    const double time_decrement = 2;

    int operation_status;

    setlocale(LC_ALL, "");
    srand(time(NULL));

    game_data data = {name_size, number_of_letters, letters, rounds, categories, min_time, time_decrement};

    clear();
    // wprintf(L"ASADASD %C\n", towupper(L'á'));

    fputws(L"Insira o número de jogadores (entre 2 e 10): ", stdout);
    fflush(stdout);

    data.number_of_players = (int)get_int(2, 10, NULL);

    clear();

    wprintf(L"Número de jogadores: %d\n", data.number_of_players);


    newline();

    operation_status = get_names(&data);

    newline();

    if (operation_status == -1)
    {
        wprintf(L"\n\tFalha ao obter nomes dos jogadores.\n\terrno (código do último erro) == %d\n", errno);
        exit(EXIT_FAILURE);
    }


    data.letters_sequence = index_permutation(data.number_of_letters);
    data.categories_sequence = index_permutation(data.rounds);

    data.round_answer = malloc(sizeof(wchar_t *) * data.number_of_players);

    data.score = calloc(data.number_of_players, sizeof(int *));

    data.time_used = calloc(data.number_of_players, sizeof(double));

    int answer_ocurrences;

    for (data.curr_round = 0; data.curr_round < data.rounds; data.curr_round++)
    {

        wprintf(L"\nPressione <Enter> para começar a %dª rodada: ", data.curr_round + 1);
        getwchar();

        clear();

        wprintf(L"Rodada %02d\n", data.curr_round + 1);

        wchar_t curr_letter = data.letters[data.letters_sequence[data.curr_round]];
        const wchar_t *curr_cat = data.categories[data.categories_sequence[data.curr_round]];

        wprintf(L"\nLetra da rodada: %C\n", curr_letter);

        wprintf(L"\nCategoria da rodada: %S\n", curr_cat);

        newline();

        data.players_sequence = index_permutation(data.number_of_players);

        putws(L"Ordem da rodada:");
        show_players(&data);

        fputws(L"\nPressione <Enter> para começar: ", stdout);
        getwchar();

        wprintf(L"sdfjisfjsidfj\n\n");

        for (data.curr_turn = 0; data.curr_turn < data.number_of_players; data.curr_turn++)
        {

            if (data.curr_round == 0) {
                data.score[data.players_sequence[data.curr_turn]] = calloc(data.rounds, sizeof(int));
                init_array(data.score[data.players_sequence[data.curr_turn]], data.rounds);
                data.time_used[data.players_sequence[data.curr_turn]] = 0;
            } 

            clear();
            set_time(&data.curr_time_left, player_total_time(&data));

            data.round_answer[data.players_sequence[data.curr_turn]] = get_answer(&data);
            wprintf(L"shit\n");


            if (data.round_answer[data.players_sequence[data.curr_turn]] == NULL)
            {
                // wprintf(L"time_left(data.curr_time_left) == %lf\n\n", time_left(data.curr_time_left));

                if (time_left(data.curr_time_left) == 0.0)
                {
                    data.round_answer[data.players_sequence[data.curr_turn]] = fwstring(L"%C", L'\0');
                }
                else
                {
                    wprintf(L"\n\tFalha ao obter resposta de %S.\n\terrno (código do último erro) == %d\n", data.player_name[data.players_sequence[data.curr_turn]], errno);
                    exit(EXIT_FAILURE);
                }
            }

            data.time_used[data.players_sequence[data.curr_turn]] += player_total_time(&data) - time_left(data.curr_time_left);

        }

        for (int p = 0; p < data.number_of_players; p++) {

            answer_ocurrences = 1;
            
            for (int i = 0; i < data.number_of_players; i++) {
                if (i != p && same_str(data.round_answer[data.players_sequence[p]], data.round_answer[data.players_sequence[i]])) {
                    answer_ocurrences++;
                }
            }

            data.score[data.players_sequence[p]][data.categories_sequence[data.curr_round]] = round(wstr_size(data.round_answer[data.players_sequence[p]])/ (double) answer_ocurrences);
        }


        clear();
        show_answers(&data);

        line_breaks(2);

        putws(L"Concluída a rodada, esta é a tabela de escores:");

        newline();

        show_scores(&data);

        free(data.players_sequence);
    }

    line_breaks(2);

    fputws(L"\nPressione <Enter> para continuar: ", stdout);
    getwchar();

    clear();

    putws(L"RESULTADO FINAL:");

    data.curr_round--;

    data.players_sequence = ascending_sequence(data.number_of_players);

    show_scores(&data);

    line_breaks(2);

    wprintf(L"Vencedor: %S.\n", data.player_name[victor(&data)]);

    for (int p = 0; p < data.number_of_players; p++) free(data.score[p]);

    free(data.score);
    free(data.letters_sequence);
    free(data.categories_sequence);
    free(data.round_answer);
    free(data.time_used);

    return EXIT_SUCCESS;
}
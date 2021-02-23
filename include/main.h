#include <wchar.h>
#include <sys/time.h>

#define putws(s) wprintf(L"%S\n", s)
#define trunc(n) ((long long) (n))
#define round(n) (trunc(n) + ((n) - trunc(n) < .5? 0: 1))
#define abs(n) ((n >= 0)? n: -n)
#define clear() system("clear")
#define newline() putwchar(L'\n');

const unsigned long WCHAR_SIZE = sizeof(wchar_t);

typedef struct timeval time_data;


typedef struct {
    const int name_size;
    const int number_of_letters;
    const wchar_t *const letters;    /* pointer to constant character variable */
    const int rounds;
    const wchar_t *const *const categories;
    const double min_time;
    const double time_decrement;
    const int answer_size;

    int curr_round; 
    int curr_turn;
    int number_of_players;
    wchar_t **player_name;    

    int *letters_sequence;
    int *players_sequence;
    int *categories_sequence;
    
    time_data curr_time_left;

    wchar_t **round_answer;
    int **score;
    int *time_used;

} game_data;

double time_left(time_data td);
void set_time(time_data *td, double sec);
wchar_t *vfwstring(const wchar_t *format, va_list ap);
int starts_with(wchar_t *s, wchar_t l);
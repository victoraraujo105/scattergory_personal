#include <wchar.h>

#define putws(s) wprintf(L"%S\n", s)

const unsigned long WCHAR_SIZE = sizeof(wchar_t);

typedef struct {
    const int name_size;
    const int number_of_letters;
    const wchar_t *const letters;    /* pointer to constant character variable */
    const int rounds;
    const wchar_t * const * const categories;
    
    int curr_round; 
    int number_of_players;
    wchar_t **player_name;    

    int *letters_sequence;
    int *players_sequence;
    
} game_data;
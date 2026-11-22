#include <stdlib.h>
#include <stdio.h>

#include "mpris.h"

int
main(int argc, char** argv)
{
    mpris_init();

    char **media_players;
    int players_count;
    mpris_load_media_player_names(&media_players, &players_count);

    for (int i = 0; i < players_count; i++) {
        printf("%s\n", media_players[i]);
        if (argc > 1 && argv[1] != NULL)
            mpris_call_and_forget(media_players[i], argv[1]);
    }

    mpris_free();
    return 0;
}

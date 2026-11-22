#ifndef _MPRIS_H
#define _MPRIS_H

// must be the first call made to this package
void mpris_init(void);
// must be the last call made to this package
void mpris_free(void);

// loads names of mediaplayers into arg0
// loads number of mediaplayers into arg1
void mpris_load_media_player_names(char ***players_dest, int *players_count);

// Send a method call to the player designated by arg0
// The method to call is designated by arg1
void mpris_call_and_forget(const char *player_name, const char *method);

#endif

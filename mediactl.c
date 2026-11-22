#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

void mpris_init(void);
void mpris_free(void);
void mpris_load_media_player_names(char ***players_dest, int *players_count);
void mpris_call_and_forget(const char *player_name, const char *method);
void panic(char *msg);
void panic_on_dbus_err(DBusError *err);

static DBusConnection *con;

// call an mpris method over DBus and return DBusMessage reply.
// BRO SERIOUSLY dont forget to unref the reply once you're done with it c:
static DBusMessage *_call_method(
        const char *destination,
        const char *path,
        const char *interface,
        const char *method)
{
    // allocate that bih here because i like polluted scopes
    char errmsg[512];

    // send the PlayPause command to the player
    DBusMessage *msg = dbus_message_new_method_call(destination, path, interface, method);
    if (msg == NULL) panic("Could not create DBus message");

    // send our message
    DBusPendingCall *call;
    dbus_bool_t success = dbus_connection_send_with_reply(con, msg, &call, 100);
    if (!success || call == NULL) {
        snprintf(errmsg, sizeof(errmsg), "DBus %s call unsuccessful", method);
        panic(errmsg);
    }

    // clean up and wait for a reply
    dbus_connection_flush(con);
    dbus_message_unref(msg);
    dbus_pending_call_block(call);

    // check for our reply
    DBusMessage *reply = dbus_pending_call_steal_reply(call);
    dbus_pending_call_unref(call);
    if (reply == NULL) {
        snprintf(errmsg, sizeof(errmsg), "Got no reply from DBus %s call", method);
        panic(errmsg);
    }

    return reply;
}

// call_method, diregard any replies
static void _call_method_and_forget(
        const char *destination,
        const char *path,
        const char *interface,
        const char *method)
{
    DBusMessage *reply = _call_method(destination, path, interface, method);
    dbus_message_unref(reply);
}

void mpris_init(void)
{
    // will check this after attempting a dbus connection
    DBusError err = {0};
    dbus_error_init(&err);

    // get connection to login-session bus daemon
    con = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
    panic_on_dbus_err(&err);
}

void mpris_free(void)
{
    // connection must be closed before dropping the last ref
    dbus_connection_close(con);
    dbus_connection_unref(con);
}

// load a list of media player names
void mpris_load_media_player_names(char ***players_dest, int *players_count)
{
    if (players_dest == NULL || players_count == NULL || con == NULL)
        panic("Bad args passed to load_media_player_names");

    (*players_dest) = NULL;
    (*players_count) = 0;

    // send the ListNames command to the DBus
    DBusMessage* reply = _call_method("org.freedesktop.DBus", "/", "org.freedesktop.DBus", "ListNames");

    // will check this after attempting to get args
    DBusError err = {0};
    dbus_error_init(&err);

    // pull list of players from reply
    int args_count;
    char **args;
    dbus_message_get_args(reply, &err, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &args, &args_count, DBUS_TYPE_INVALID);
    panic_on_dbus_err(&err);

    // filter for mediaplayers implementing mpris
    const char* mpris_namespace = "org.mpris.MediaPlayer2";
    const int len = strlen(mpris_namespace);

    for (int i = 0; i < args_count; ++i) {
        // check namespace matches
        if (strncmp(args[i], mpris_namespace, len) == 0) {
            (*players_count) += 1;

            // dynamically build a list of the matching strings on the heap
            size_t new_size = sizeof(args[i]) * (*players_count);
            if ((*players_dest) == NULL) (*players_dest) = malloc(new_size);
            else (*players_dest) = realloc(*players_dest, new_size);
            (*players_dest)[(*players_count)-1] = args[i];
        }
    }
    dbus_message_unref(reply);
}

void mpris_call_and_forget(const char *player_name, const char *method)
{
    _call_method_and_forget(player_name, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", method);
}

void panic(char *msg)
{
    if (msg != NULL) printf("mediactl error:\n\t%s\n", msg);
    else printf("mediactl encountered an unexpected error, exiting...\n");
    exit(1);
}

void panic_on_dbus_err(DBusError *err)
{
    char msg[512];
    if (dbus_error_is_set(err)) {
        snprintf(msg, sizeof(msg), "D-Bus Error:\n\t%s\n\t%s\n", err->name, err->message);
        dbus_error_free(err);
        panic(msg);
    }
}

int main(int argc, char** argv)
{
    mpris_init();

    char **media_players;
    int players_count;
    mpris_load_media_player_names(&media_players, &players_count);

    for (int i = 0; i < players_count; ++i) {
        printf("%s\n", media_players[i]);
        if (argc > 1 && argv[1] != NULL)
            mpris_call_and_forget(media_players[i], argv[1]);
    }

    mpris_free();
    return 0;
}

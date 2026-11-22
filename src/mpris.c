#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dbus/dbus.h>

#include "panic.h"


static DBusConnection *con;

// call an mpris method over DBus and return DBusMessage reply.
// BRO SERIOUSLY dont forget to unref the reply once you're done with it c:
static DBusMessage*
call_method(
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
    DBusMessage* reply = dbus_pending_call_steal_reply(call);
    dbus_pending_call_unref(call);
    if (reply == NULL) {
        snprintf(errmsg, sizeof(errmsg), "Got no reply from DBus %s call", method);
        panic(errmsg);
    }

    return reply;
}

// call_method, diregard any replies
static void
call_method_and_forget(
        const char *destination,
        const char *path,
        const char *interface,
        const char *method)
{
    DBusMessage* reply = call_method(destination, path, interface, method);
    dbus_message_unref(reply);
}


/*
                    HEADER DEFINITIONS
*/

void
mpris_init(void)
{
    // will check this after attempting a dbus connection
    DBusError err = {0};
    dbus_error_init(&err);

    // get connection to login-session bus daemon
    con = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
    panic_on_dbus_err(&err);
}

void
mpris_free(void)
{
    // connection must be closed before dropping the last ref
    dbus_connection_close(con);
    dbus_connection_unref(con);
}

// load a list of media player names
void
mpris_load_media_player_names(char ***players_dest, int *players_count)
{
    if (players_dest == NULL || players_count == NULL || con == NULL)
        panic("Bad args passed to loadMediaPlayerNames");

    (*players_dest) = NULL;
    (*players_count) = 0;

    // send the ListNames command to the DBus
    DBusMessage* reply = call_method(
        "org.freedesktop.DBus", //destination
        "/",                    // path
        "org.freedesktop.DBus", // interface
        "ListNames");           // method

    // will check this after attempting to get args
    DBusError err = {0};
    dbus_error_init(&err);

    // pull list of players from reply
    int m_argc;
    char **args;
    dbus_message_get_args(reply, &err, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &args, &m_argc, DBUS_TYPE_INVALID);
    panic_on_dbus_err(&err);

    // filter for mediaplayers implementing mpris
    const char* mpris_namespace = "org.mpris.MediaPlayer2";
    const int len = strlen(mpris_namespace);

    for (int i = 0; i < m_argc; ++i) {
        // check namespace matches
        if (strncmp(args[i], mpris_namespace, len) == 0){
            (*players_count) += 1;

            // dynamically build a list of the matching strings on the heap
            size_t new_size = sizeof(args[i]) * (*players_count);

            if ((*players_dest) == NULL)
                (*players_dest) = malloc(new_size);
            else
                (*players_dest) = realloc(*players_dest, new_size);

            (*players_dest)[(*players_count)-1] = args[i];
        }
    }
    dbus_message_unref(reply);
}

void
mpris_call_and_forget(const char *player_name, const char *method)
{
    call_method_and_forget(player_name, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", method);
}


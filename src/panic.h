#ifndef _PANIC_H
#define _PANIC_H

#include <dbus/dbus.h>

void panic(char *msg);
void panic_on_dbus_err(DBusError *err);

#endif

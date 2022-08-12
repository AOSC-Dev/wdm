/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006-2007 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2007 Kevin Kofler <Kevin@tigcc.ticalc.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <dm.h>
#include <dm_auth.h>
#include <wdmlib.h>

#include <stdlib.h>
#include <string.h>
#include <pwd.h>

#include <sys/types.h>
#include <sys/sysmacros.h>

#ifdef WITH_CONSOLE_KIT
#include <consolekit.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

#define CK_NAME              "org.freedesktop.ConsoleKit"
#define CK_PATH              "/org/freedesktop/ConsoleKit"
#define CK_INTERFACE         "org.freedesktop.ConsoleKit"
#define CK_MANAGER_PATH      "/org/freedesktop/ConsoleKit/Manager"
#define CK_MANAGER_INTERFACE "org.freedesktop.ConsoleKit.Manager"
#define CK_SESSION_INTERFACE "org.freedesktop.ConsoleKit.Session"

static DBusConnection *private_connection = NULL;

static void freeStrArr(char **arr)
{
	char **tarr;

	if (arr) {
		for (tarr = arr; *tarr; tarr++)
			free(*tarr);
		free(arr);
	}
}

static void add_param_int(DBusMessageIter * iter_struct, const char *key, int value)
{
	DBusMessageIter iter_struct_entry;
	DBusMessageIter iter_var;

	dbus_message_iter_open_container(iter_struct, DBUS_TYPE_STRUCT, NULL, &iter_struct_entry);

	dbus_message_iter_append_basic(&iter_struct_entry, DBUS_TYPE_STRING, &key);

	dbus_message_iter_open_container(&iter_struct_entry, DBUS_TYPE_VARIANT, DBUS_TYPE_INT32_AS_STRING, &iter_var);

	dbus_message_iter_append_basic(&iter_var, DBUS_TYPE_INT32, &value);

	dbus_message_iter_close_container(&iter_struct_entry, &iter_var);

	dbus_message_iter_close_container(iter_struct, &iter_struct_entry);
}

static void add_param_boolean(DBusMessageIter * iter_struct, const char *key, int value)
{
	DBusMessageIter iter_struct_entry;
	DBusMessageIter iter_var;

	dbus_message_iter_open_container(iter_struct, DBUS_TYPE_STRUCT, NULL, &iter_struct_entry);

	dbus_message_iter_append_basic(&iter_struct_entry, DBUS_TYPE_STRING, &key);

	dbus_message_iter_open_container(&iter_struct_entry, DBUS_TYPE_VARIANT, DBUS_TYPE_BOOLEAN_AS_STRING, &iter_var);

	dbus_message_iter_append_basic(&iter_var, DBUS_TYPE_BOOLEAN, &value);

	dbus_message_iter_close_container(&iter_struct_entry, &iter_var);

	dbus_message_iter_close_container(iter_struct, &iter_struct_entry);
}

static void add_param_string(DBusMessageIter * iter_struct, const char *key, const char *value)
{
	DBusMessageIter iter_struct_entry;
	DBusMessageIter iter_var;

	dbus_message_iter_open_container(iter_struct, DBUS_TYPE_STRUCT, NULL, &iter_struct_entry);

	dbus_message_iter_append_basic(&iter_struct_entry, DBUS_TYPE_STRING, &key);

	dbus_message_iter_open_container(&iter_struct_entry, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &iter_var);

	dbus_message_iter_append_basic(&iter_var, DBUS_TYPE_STRING, &value);

	dbus_message_iter_close_container(&iter_struct_entry, &iter_var);

	dbus_message_iter_close_container(iter_struct, &iter_struct_entry);
}

static int session_get_x11_display(DBusConnection * connection, const char *ssid, char **str)
{
	DBusError error;
	DBusMessage *message;
	DBusMessage *reply;
	DBusMessageIter iter;
	const char *value;

	if (str != NULL) {
		*str = NULL;
	}

	message = dbus_message_new_method_call(CK_NAME, ssid, CK_SESSION_INTERFACE, "GetX11Display");
	if (message == NULL) {
		WDMDebug("ConsoleKit: Couldn't allocate the D-Bus message");
		return FALSE;
	}

	dbus_error_init(&error);
	reply = dbus_connection_send_with_reply_and_block(connection, message, -1, &error);
	if (dbus_error_is_set(&error)) {
		WDMDebug("ConsoleKit: %s raised:\n %s\n\n", error.name, error.message);
		reply = NULL;
	}

	dbus_connection_flush(connection);
	dbus_message_unref(message);

	if (reply == NULL) {
		return FALSE;
	}

	dbus_message_iter_init(reply, &iter);
	dbus_message_iter_get_basic(&iter, &value);
	if (str != NULL) {
		*str = strdup(value);
	}
	dbus_message_unref(reply);

	return TRUE;
}

static int session_unlock(DBusConnection * connection, const char *ssid)
{
	DBusError error;
	DBusMessage *message;
	DBusMessage *reply;

	WDMDebug("ConsoleKit: Unlocking session %s", ssid);
	message = dbus_message_new_method_call(CK_NAME, ssid, CK_SESSION_INTERFACE, "Unlock");
	if (message == NULL) {
		WDMDebug("ConsoleKit: Couldn't allocate the D-Bus message");
		return FALSE;
	}

	dbus_error_init(&error);
	reply = dbus_connection_send_with_reply_and_block(connection, message, -1, &error);
	dbus_message_unref(message);
	dbus_message_unref(reply);
	dbus_connection_flush(connection);

	if (dbus_error_is_set(&error)) {
		WDMDebug("ConsoleKit: %s raised:\n %s\n\n", error.name, error.message);
		return FALSE;
	}

	return TRUE;
}

/* from libhal */
static char **get_path_array_from_iter(DBusMessageIter * iter, int *num_elements)
{
	int count;
	char **buffer;

	count = 0;
	buffer = (char **)malloc(sizeof(char *) * 8);

	if (buffer == NULL)
		goto oom;

	buffer[0] = NULL;
	while (dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_OBJECT_PATH) {
		const char *value;
		char *str;

		if ((count % 8) == 0 && count != 0) {
			buffer = realloc(buffer, sizeof(char *) * (count + 8));
			if (buffer == NULL)
				goto oom;
		}

		dbus_message_iter_get_basic(iter, &value);
		str = strdup(value);
		if (str == NULL)
			goto oom;

		buffer[count] = str;

		dbus_message_iter_next(iter);
		count++;
	}

	if ((count % 8) == 0) {
		buffer = realloc(buffer, sizeof(char *) * (count + 1));
		if (buffer == NULL)
			goto oom;
	}

	buffer[count] = NULL;
	if (num_elements != NULL)
		*num_elements = count;
	return buffer;

 oom:
	WDMError("%s %d : error allocating memory\n", __FILE__, __LINE__);
	return NULL;

}

static char **get_sessions_for_user(DBusConnection * connection, const char *user, const char *x11_display)
{
	DBusError error;
	DBusMessage *message;
	DBusMessage *reply;
	DBusMessageIter iter;
	DBusMessageIter iter_reply;
	DBusMessageIter iter_array;
	struct passwd *pwent;
	char **sessions;

	sessions = NULL;
	message = NULL;
	reply = NULL;

	pwent = getpwnam(user);

	dbus_error_init(&error);
	message = dbus_message_new_method_call(CK_NAME, CK_MANAGER_PATH, CK_MANAGER_INTERFACE, "GetSessionsForUser");
	if (message == NULL) {
		WDMDebug("ConsoleKit: Couldn't allocate the D-Bus message");
		goto out;
	}

	dbus_message_iter_init_append(message, &iter);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &pwent->pw_uid);

	dbus_error_init(&error);
	reply = dbus_connection_send_with_reply_and_block(connection, message, -1, &error);
	dbus_connection_flush(connection);

	if (dbus_error_is_set(&error)) {
		WDMDebug("ConsoleKit: %s raised:\n %s\n\n", error.name, error.message);
		goto out;
	}

	if (reply == NULL) {
		WDMDebug("ConsoleKit: No reply for GetSessionsForUser");
		goto out;
	}

	dbus_message_iter_init(reply, &iter_reply);
	if (dbus_message_iter_get_arg_type(&iter_reply) != DBUS_TYPE_ARRAY) {
		WDMDebug("ConsoleKit: Wrong reply for GetSessionsForUser - expecting an array.");
		goto out;
	}

	dbus_message_iter_recurse(&iter_reply, &iter_array);
	sessions = get_path_array_from_iter(&iter_array, NULL);

 out:
	if (message != NULL) {
		dbus_message_unref(message);
	}
	if (reply != NULL) {
		dbus_message_unref(reply);
	}

	return sessions;
}

void unlock_ck_session(const char *user, const char *x11_display)
{
	DBusError error;
	DBusConnection *connection;
	char **sessions;
	int i;

	WDMDebug("ConsoleKit: Unlocking session for %s on %s", user, x11_display);

	dbus_error_init(&error);
	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
	if (connection == NULL) {
		WDMDebug("ConsoleKit: Failed to connect to the D-Bus daemon: %s", error.message);
		dbus_error_free(&error);
		return;
	}

	sessions = get_sessions_for_user(connection, user, x11_display);
	if (sessions == NULL || sessions[0] == NULL) {
		WDMDebug("ConsoleKit: no sessions found");
		return;
	}

	for (i = 0; sessions[i] != NULL; i++) {
		char *ssid;
		char *xdisplay;

		ssid = sessions[i];
		session_get_x11_display(connection, ssid, &xdisplay);
		WDMDebug("ConsoleKit: session %s has DISPLAY %s", ssid, xdisplay);

		if (xdisplay != NULL && x11_display != NULL && strcmp(xdisplay, x11_display) == 0) {
			int res;

			res = session_unlock(connection, ssid);
			if (!res) {
				WDMError("ConsoleKit: Unable to unlock %s", ssid);
			}
		}

		free(xdisplay);
	}

	freeStrArr(sessions);
}

char *open_ck_session(struct passwd *pwent, struct display *d)
{
	DBusConnection *connection;
	DBusError error;
	DBusMessage *message;
	DBusMessage *reply;
	DBusMessageIter iter;
	DBusMessageIter iter_struct;
	char *cookie;

	cookie = NULL;

	WDMDebug("ConsoleKit: Opening session for %s", pwent->pw_name);

	dbus_error_init(&error);
	connection = dbus_bus_get_private(DBUS_BUS_SYSTEM, &error);
	private_connection = connection;

	if (connection == NULL) {
		WDMDebug("ConsoleKit: Failed to connect to the D-Bus daemon: %s", error.message);
		dbus_error_free(&error);
		return NULL;
	}

	dbus_connection_set_exit_on_disconnect(connection, FALSE);
	/* FIXME: What to do about these?
	   dbus_connection_set_watch_functions( connection,
	   dbusAddWatch,
	   dbusRemoveWatch,
	   dbusToggleWatch,
	   data, 0 );
	   dbus_connection_set_timeout_functions( connection,
	   dbusAddTimeout,
	   dbusRemoveTimeout,
	   dbusToggleTimeout,
	   data, 0 );
	   dbus_connection_set_wakeup_main_function( connection,
	   dbusWakeupMain,
	   data, 0 ); */

	dbus_error_init(&error);
	message = dbus_message_new_method_call(CK_NAME, CK_MANAGER_PATH, CK_MANAGER_INTERFACE, "OpenSessionWithParameters");
	if (message == NULL) {
		WDMDebug("ConsoleKit: Couldn't allocate the D-Bus message");
		return NULL;
	}

	dbus_message_iter_init_append(message, &iter);
	dbus_message_iter_open_container(&iter,
									 DBUS_TYPE_ARRAY,
									 DBUS_STRUCT_BEGIN_CHAR_AS_STRING
									 DBUS_TYPE_STRING_AS_STRING
									 DBUS_TYPE_VARIANT_AS_STRING DBUS_STRUCT_END_CHAR_AS_STRING, &iter_struct);

	add_param_int(&iter_struct, "user", pwent->pw_uid);
	add_param_string(&iter_struct, "x11-display", d->name);
	add_param_boolean(&iter_struct, "is-local", d->displayType.location);
#ifdef XDMCP
	if (!(d->displayType.location) && d->name) {
		int length;
		char *ptr, name[256];

		ptr = strchr(d->name, ':');
		length = ptr ? ptr - d->name + 1 : sizeof(name);
		snprintf(name, length, "%s", d->name);
		add_param_string(&iter_struct, "remote-host-name", name);
	}
#endif

	if (d->serverPid > 0) {
		int tty;
		FILE *fp;
		char buffer[32];

		sprintf(buffer, "/proc/%d/stat", d->serverPid);
		if ((fp = fopen(buffer, "r")) != NULL) {
			if (fscanf(fp, "%*d %*s %*c %*d %*d %*d %d", &tty) == 1 && major(tty) == 4 && minor(tty) < 64) {
				sprintf(buffer, "/dev/tty%d", minor(tty));
				add_param_string(&iter_struct, "x11-display-device", buffer);
			}
			fclose(fp);
		}
	}

	dbus_message_iter_close_container(&iter, &iter_struct);

	reply = dbus_connection_send_with_reply_and_block(connection, message, -1, &error);
	if (dbus_error_is_set(&error)) {
		WDMDebug("ConsoleKit: %s raised:\n %s\n\n", error.name, error.message);
		reply = NULL;
	}

	dbus_connection_flush(connection);

	dbus_message_unref(message);
	dbus_error_free(&error);

	if (reply != NULL) {
		const char *value;

		dbus_message_iter_init(reply, &iter);
		dbus_message_iter_get_basic(&iter, &value);
		cookie = strdup(value);
		dbus_message_unref(reply);
	}

	return cookie;
}

void close_ck_session(const char *cookie)
{
	DBusError error;
	DBusMessage *message;
	DBusMessage *reply;
	DBusMessageIter iter;

	if (cookie == NULL) {
		return;
	}

	if (private_connection == NULL) {
		return;
	}

	dbus_error_init(&error);
	message = dbus_message_new_method_call(CK_NAME, CK_MANAGER_PATH, CK_MANAGER_INTERFACE, "CloseSession");
	if (message == NULL) {
		WDMDebug("ConsoleKit: Couldn't allocate the D-Bus message");
		return;
	}

	dbus_message_iter_init_append(message, &iter);
	dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &cookie);

	reply = dbus_connection_send_with_reply_and_block(private_connection, message, -1, &error);
	if (dbus_error_is_set(&error)) {
		WDMDebug("ConsoleKit: %s raised:\n %s\n\n", error.name, error.message);
		reply = NULL;
	}

	dbus_connection_flush(private_connection);

	dbus_message_unref(message);
	dbus_error_free(&error);

	dbus_connection_close(private_connection);
	private_connection = NULL;
}
#endif							/* WITH_CONSOLE_KIT */

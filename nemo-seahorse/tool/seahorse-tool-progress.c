/*
 * Seahorse
 *
 * Copyright (C) 2006 Stefan Walter
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>

#include "seahorse-tool.h"
#include "seahorse-util.h"
#include "seahorse-progress.h"
#include "seahorse-widget.h"

#define PROGRESS_DELAY 1500
#define PROGRESS_ARG "--show-internal-progress-window"
#define CMD_BLOCK "BLOCK"
#define CMD_UNBLOCK "UNBLOCK"
#define CMD_PROGRESS "PROGRESS"
#define STDOUT 1

/* -----------------------------------------------------------------------------
 * RUNS IN PROGRESS PROCESS
 */

/* Title to display in progress */
static const gchar *progress_title = NULL;

/* Don't show progress, until TRUE */
static gboolean progress_block = FALSE;

/* Whether progress has been shown or not */
static gboolean progress_visible = FALSE;

static gboolean
progress_show (SeahorseOperation *op)
{
    if (progress_visible || progress_block)
        return FALSE;

    seahorse_progress_show (op, progress_title, FALSE);
    progress_visible = TRUE;

    /* Don't fire this again */
    return FALSE;
}
static void
done_handler (SeahorseOperation *op, gpointer userdata)
{
    gtk_main_quit ();
}

static void
process_line (SeahorseOperation *op, const gchar *line)
{
    gchar **args = g_strsplit_set (line, "\n ", 3);
    gdouble fract;
    gchar *e, *message = NULL;

    if (!args[0])
        g_warning ("invalid progress line");

    else if (g_ascii_strcasecmp (args[0], CMD_BLOCK) == 0)
        progress_block = TRUE;

    else if (g_ascii_strcasecmp (args[0], CMD_UNBLOCK) == 0) {
        progress_block = FALSE;
        g_timeout_add (PROGRESS_DELAY, (GSourceFunc)progress_show, op);
    }

    else if (g_ascii_strcasecmp (args[0], CMD_PROGRESS) == 0) {

        if (args[1]) {
            fract = strtod (args[1], &e);
            if(*e || fract < -1 || fract > 1.0)
                g_warning ("invalid progress format");
            else {

                if (args[2]) {
                    message = args[2];
                    g_strstrip (message);
                }

                seahorse_operation_mark_progress (op,
                        message && message[0] ? message : NULL, fract);
            }
        }
    }

    else
        g_warning ("invalid progress command");

    g_strfreev (args);
}

static gboolean
io_handler (GIOChannel *source, GIOCondition condition,
            SeahorseOperation *op)
{
    gchar *line;
    gsize length;
    GError *err = NULL;

    if (condition & G_IO_IN) {

        /*
         * The input format is:
         *
         * 0.5 Progress messsage
         */

        /* Read 1 line from the io channel, including newline character */
        g_io_channel_read_line (source, &line, &length, NULL, &err);

        if (err != NULL) {
            g_critical ("couldn't read from socket: %s", err->message);
            gtk_main_quit ();
            return FALSE;
        }

        if (length > 0)
            process_line (op, line);

        if (line)
            g_free (line);
    }

    if (condition & G_IO_HUP) {
        gtk_main_quit ();
        return FALSE;
    }

    return TRUE;
}

static int
progress_main (int argc, char* argv[])
{
    SeahorseOperation *op;
    GIOChannel *io;

    gtk_init (&argc, &argv);

    op = g_object_new (SEAHORSE_TYPE_OPERATION, NULL);
    seahorse_operation_mark_start (op);

    /* Watch for done (ie: in this case only cancel) */
    g_signal_connect (op, "done", G_CALLBACK (done_handler), NULL);

    /* Hook up an IO channel to stdin */
    io = g_io_channel_unix_new (0);
    g_io_channel_set_encoding (io, NULL, NULL);
    g_io_add_watch (io, G_IO_IN | G_IO_HUP, (GIOFunc)io_handler, op);

    progress_visible = FALSE;
    progress_title = argc > 2 ? argv[2] : NULL;
    g_timeout_add (PROGRESS_DELAY, (GSourceFunc)progress_show, op);

    gtk_main ();

    if (seahorse_operation_is_running (op))
        seahorse_operation_mark_done (op, FALSE, NULL);

    return 0;
}

/* -----------------------------------------------------------------------------
 * RUNS IN PARENT PROCESS
 */

/* Program to run for progress */
static const gchar *progress_binary = NULL;

/* PID if child process */
static GPid progress_pid = -1;

/* FD to write progress updates to */
static int progress_fd = -1;

/* Whether child has cancelled or not */
static gboolean cancelled = FALSE;

static void
progress_cancel (GPid pid, gint status, gpointer data)
{
    /* TODO: Should we send ourselves a signal? */
    cancelled = TRUE;
}

void
seahorse_tool_progress_start (const gchar *title)
{
    GError *err = NULL;
    gboolean ret;
    gchar* argv[4];

    argv[0] = (gchar *)progress_binary;
    argv[1] = PROGRESS_ARG;
    argv[2] = (gchar *)title;
    argv[3] = NULL;

    ret = g_spawn_async_with_pipes (NULL, argv, NULL, G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                                    NULL, NULL, &progress_pid, &progress_fd, NULL, NULL, &err);

    if (!ret) {
        g_warning ("couldn't start progress process: %s", err ? err->message : "");
        progress_pid = -1;
        progress_fd = -1;
        return;
    }

    g_child_watch_add (progress_pid, progress_cancel, NULL);
}

void
seahorse_tool_progress_init (int argc, char* argv[])
{
    int r;

    /* Progress mode */
    if (argc >= 2 && g_strcmp0 (argv[1], PROGRESS_ARG) == 0) {
        r = progress_main (argc, argv);
        exit (r);
    }

    /* Normal mode */
    progress_binary = argv[0];
}

gboolean
seahorse_tool_progress_check (void)
{
    /* Process all events necessary for this check */
    while (g_main_context_iteration (NULL, FALSE));
    return !cancelled;
}

void
seahorse_tool_progress_block (gboolean block)
{
    if (progress_fd != -1)
        seahorse_util_printf_fd (progress_fd, "%s \n", block ? CMD_BLOCK : CMD_UNBLOCK);
}

gboolean
seahorse_tool_progress_update (gdouble fract, const gchar *message)
{
    if (progress_fd != -1) {
        if (!seahorse_util_printf_fd (progress_fd, "%s %0.2f %s\n", CMD_PROGRESS,
                                      fract, message ? message : "")) {
            cancelled = TRUE;

            return FALSE;
        }
    }

    return seahorse_tool_progress_check ();
}

void
seahorse_tool_progress_stop ()
{
    if (progress_pid != -1) {
        kill (progress_pid, SIGTERM);
        g_spawn_close_pid (progress_pid);
        progress_pid = -1;
    }
}


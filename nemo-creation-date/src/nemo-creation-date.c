/* nemo-creation-date -- Nemo file creation date extension
 *
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
 * Foundation, Inc., 51 Franklin Street - Suite 500, Boston, MA 02110-1335, USA.
 *
 */

#include <config.h>

#include <libnemo-extension/nemo-extension-types.h>
#include <libnemo-extension/nemo-column-provider.h>
#include <libnemo-extension/nemo-file-info.h>
#include <libnemo-extension/nemo-info-provider.h>
#include <libnemo-extension/nemo-name-and-desc-provider.h>

#include "nemo-creation-date.h"
#include "statx.h"

#include <glib/gi18n-lib.h>
#include <gio/gio.h>

#include <string.h>
#include <time.h>

static void nemo_creation_date_column_provider_iface_init (NemoColumnProviderIface *iface);
static void nemo_creation_date_info_provider_iface_init (NemoInfoProviderIface *iface);
static void nemo_creation_date_nd_provider_iface_init (NemoNameAndDescProviderIface *iface);

struct _NemoCreationDate {
    GObject parent_object;
    GSettings *nemo_settings;
    GSettings *cinnamon_interface_settings;

    gint format_type;
    gboolean use_24h;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED (NemoCreationDate,
                                nemo_creation_date,
                                G_TYPE_OBJECT,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (NEMO_TYPE_INFO_PROVIDER,
                                                               nemo_creation_date_info_provider_iface_init)
                                G_IMPLEMENT_INTERFACE_DYNAMIC (NEMO_TYPE_COLUMN_PROVIDER,
                                                               nemo_creation_date_column_provider_iface_init)
                                G_IMPLEMENT_INTERFACE_DYNAMIC (NEMO_TYPE_NAME_AND_DESC_PROVIDER,
                                                               nemo_creation_date_nd_provider_iface_init));

typedef struct {
  gboolean cancelled;
  NemoInfoProvider *provider;
  NemoFileInfo *file;
  GClosure *update_complete;
} NemoCreationDateHandle;

typedef enum
{
    NEMO_DATE_FORMAT_LOCALE,
    NEMO_DATE_FORMAT_ISO,
    NEMO_DATE_FORMAT_INFORMAL
} NemoDateFormat;

static GList *
nemo_creation_date_get_columns (NemoColumnProvider *provider)
{
    NemoColumn *column;
    GList *columns;

    column = nemo_column_new ("nemo-creation-date-column",
                              "creation-date",
                              _("Date Created"),
                              _("The date the file was created"));

    columns = g_list_append (NULL, column);

    column = nemo_column_new ("nemo-creation-date-column-with-time",
                              "creation-date-with-time",
                              _("Created - Time"),
                              _("The date and time the file was created"));

    columns = g_list_append (columns, column);

    return columns;
}


/* eel_str_replace_substring */
char *
str_replace_substring (const char *string,
                       const char *substring,
                       const char *replacement)
{
    int substring_length, replacement_length, result_length, remaining_length;
    const char *p, *substring_position;
    char *result, *result_position;

    g_return_val_if_fail (substring != NULL, g_strdup (string));
    g_return_val_if_fail (substring[0] != '\0', g_strdup (string));

    if (string == NULL) {
        return NULL;
    }

    substring_length = substring ? strlen (substring) : 0;
    replacement_length = replacement ? strlen (replacement) : 0;

    result_length = strlen (string);
    for (p = string; ; p = substring_position + substring_length) {
        substring_position = strstr (p, substring);
        if (substring_position == NULL) {
            break;
        }
        result_length += replacement_length - substring_length;
    }

    result = g_malloc (result_length + 1);

    result_position = result;
    for (p = string; ; p = substring_position + substring_length) {
        substring_position = strstr (p, substring);
        if (substring_position == NULL) {
            remaining_length = strlen (p);
            memcpy (result_position, p, remaining_length);
            result_position += remaining_length;
            break;
        }
        memcpy (result_position, p, substring_position - p);
        result_position += substring_position - p;
        memcpy (result_position, replacement, replacement_length);
        result_position += replacement_length;
    }
    g_assert (result_position - result == result_length);
    result_position[0] = '\0';

    return result;
}

static NemoOperationResult
nemo_creation_date_update_file_info (NemoInfoProvider     *provider,
                                     NemoFileInfo         *file,
                                     GClosure             *update_complete,
                                     NemoOperationHandle **handle)
{
    NemoCreationDate *cd;
    GFile *location;
    GDateTime *file_date_time, *file_date, *now, *today_midnight;
    gchar *path;
    gchar *formatted, *formatted_time;
    gchar *result_with_ratio;
    const gchar *format, *format_time;
    gint days_ago;
    gboolean use_24;
    time_t btime;

    cd = (NemoCreationDate *) provider;

    location = nemo_file_info_get_location (file);

    if (!g_file_is_native (location))
    {
        nemo_file_info_add_string_attribute (file, "creation-date", "");
        nemo_file_info_add_string_attribute (file, "creation-date-with-time", "");
        g_object_unref (location);

        return NEMO_OPERATION_COMPLETE;
    }

    path = g_file_get_path (location);
    btime = get_file_btime (path);

    g_free (path);
    g_object_unref (location);

    if (btime == 0)
    {
        nemo_file_info_add_string_attribute (file, "creation-date", "");
        nemo_file_info_add_string_attribute (file, "creation-date-with-time", "");

        return NEMO_OPERATION_COMPLETE;
    }

    file_date_time = g_date_time_new_from_unix_local (btime);

    if (cd->format_type == NEMO_DATE_FORMAT_LOCALE)
    { 
        formatted = g_date_time_format (file_date_time, "%c");
        formatted_time = g_strdup (formatted);
        goto out;
    }
    else
    if (cd->format_type == NEMO_DATE_FORMAT_ISO)
    {
        formatted = g_date_time_format (file_date_time, "%Y-%m-%d %H:%M:%S");
        formatted_time = g_strdup (formatted);
        goto out;
    }

    now = g_date_time_new_now_local ();
    today_midnight = g_date_time_new_local (g_date_time_get_year (now),
                                            g_date_time_get_month (now),
                                            g_date_time_get_day_of_month (now),
                                            0, 0, 0);

    file_date = g_date_time_new_local (g_date_time_get_year (file_date_time),
                                       g_date_time_get_month (file_date_time),
                                       g_date_time_get_day_of_month (file_date_time),
                                       0, 0, 0);

    days_ago = g_date_time_difference (today_midnight, file_date) / G_TIME_SPAN_DAY;

    use_24 = cd->use_24h;

    // Show only the time if date is on today
    if (days_ago < 1) {
        if (use_24) {
            /* Translators: Time in 24h format */
            format = format_time = _("%H:%M");
        } else {
            /* Translators: Time in 12h format */
            format = format_time = _("%l:%M %p");
        }
    }
    // Show the word "Yesterday" and time if date is on yesterday
    else if (days_ago < 2) {
            // xgettext:no-c-format
        format = _("Yesterday");
        if (use_24) {
            /* Translators: this is the word Yesterday followed by
             * a time in 24h format. i.e. "Yesterday 23:04" */
            // xgettext:no-c-format
            format_time = _("Yesterday %H:%M");
        } else {
            /* Translators: this is the word Yesterday followed by
             * a time in 12h format. i.e. "Yesterday 9:04 PM" */
            // xgettext:no-c-format
            format_time = _("Yesterday %l:%M %p");
        }
    }
    // Show a week day and time if date is in the last week
    else if (days_ago < 7) {
        // xgettext:no-c-format
        format = _("%A");
        if (use_24) {
            /* Translators: this is the name of the week day followed by
             * a time in 24h format. i.e. "Monday 23:04" */
            // xgettext:no-c-format
            format_time = _("%A %H:%M");
        } else {
            /* Translators: this is the week day name followed by
             * a time in 12h format. i.e. "Monday 9:04 PM" */
            // xgettext:no-c-format
            format_time = _("%A %l:%M %p");
        }
    } else if (g_date_time_get_year (file_date) == g_date_time_get_year (now)) {
        /* Translators: this is the day of the month followed
         * by the abbreviated month name i.e. "3 February" */
        // xgettext:no-c-format
        format = _("%-e %B");
        if (use_24) {
            /* Translators: this is the day of the month followed
             * by the abbreviated month name followed by a time in
             * 24h format i.e. "3 February 23:04" */
            // xgettext:no-c-format
            format_time = _("%-e %B %H:%M");
        } else {
            /* Translators: this is the day of the month followed
             * by the abbreviated month name followed by a time in
             * 12h format i.e. "3 February 9:04" */
            // xgettext:no-c-format
            format_time = _("%-e %B %l:%M %p");
        }
    } else {
        /* Translators: this is the day of the month followed by the abbreviated
         * month name followed by the year i.e. "3 Feb 2015" */
        // xgettext:no-c-format
        format = _("%-e %b %Y");
        if (use_24) {
            /* Translators: this is the day number followed
             * by the abbreviated month name followed by the year followed
             * by a time in 24h format i.e. "3 Feb 2015 23:04" */
            // xgettext:no-c-format
            format_time = _("%-e %b %Y %H:%M");
        } else {
            /* Translators: this is the day number followed
             * by the abbreviated month name followed by the year followed
             * by a time in 12h format i.e. "3 Feb 2015 9:04 PM" */
            // xgettext:no-c-format
            format_time = _("%-e %b %Y %l:%M %p");
        }
    }

    g_date_time_unref (file_date);
    g_date_time_unref (now);
    g_date_time_unref (today_midnight);

    formatted = g_date_time_format (file_date_time, format);
    formatted_time = g_date_time_format (file_date_time, format_time);

    g_date_time_unref (file_date_time);

out:

    /* Replace ":" with ratio. Replacement is done afterward because g_date_time_format
     * may fail with utf8 chars in some locales */
    result_with_ratio = str_replace_substring (formatted, ":", "∶");
    nemo_file_info_add_string_attribute (file, "creation-date", result_with_ratio);

    g_clear_pointer (&result_with_ratio, g_free);

    result_with_ratio = str_replace_substring (formatted_time, ":", "∶");
    nemo_file_info_add_string_attribute (file, "creation-date-with-time", result_with_ratio);

    g_free (result_with_ratio);
    g_free (formatted);
    g_free (formatted_time);

    return NEMO_OPERATION_COMPLETE;
}

static void
nemo_creation_date_cancel_update (NemoInfoProvider *provider,
                                  NemoOperationHandle *handle)
{
    NemoCreationDateHandle *cd_handle;
    
    cd_handle = (NemoCreationDateHandle *) handle;
    cd_handle->cancelled = TRUE;
}

static GList *
nemo_creation_date_get_name_and_desc (NemoNameAndDescProvider *provider)
{
    GList *ret = NULL;

    ret = g_list_append (ret, _("Nemo Creation Date:::Shows file creation data in a list view column"));

    return ret;
}

#define NEMO_PREFERENCES_DATE_FORMAT "date-format"
#define USE_24H "clock-use-24h"

static void
on_date_format_setting_changed (NemoCreationDate *cd)
{
    cd->format_type = g_settings_get_enum (cd->nemo_settings,
                                           NEMO_PREFERENCES_DATE_FORMAT);
    cd->use_24h = g_settings_get_boolean (cd->cinnamon_interface_settings,
                                          USE_24H);
}

static void
nemo_creation_date_finalize (GObject *object)
{
    NemoCreationDate *cd = (NemoCreationDate *) object;

    g_clear_object (&cd->nemo_settings);
    g_clear_object (&cd->cinnamon_interface_settings);

    G_OBJECT_CLASS (nemo_creation_date_parent_class)->finalize (object);
}

static void
nemo_creation_date_init (NemoCreationDate *cd)
{
    cd->nemo_settings = g_settings_new("org.nemo.preferences");
    cd->cinnamon_interface_settings = g_settings_new ("org.cinnamon.desktop.interface");

    g_signal_connect_swapped (cd->nemo_settings,
                              "changed::" NEMO_PREFERENCES_DATE_FORMAT,
                              (GCallback) on_date_format_setting_changed,
                              cd);

    g_signal_connect_swapped (cd->cinnamon_interface_settings,
                              "changed::" USE_24H,
                              (GCallback) on_date_format_setting_changed,
                              cd);

    on_date_format_setting_changed (cd);
}

static void
nemo_creation_date_column_provider_iface_init (NemoColumnProviderIface *iface)
{
    iface->get_columns = nemo_creation_date_get_columns;
}

static void 
nemo_creation_date_info_provider_iface_init (NemoInfoProviderIface *iface)
{
    iface->update_file_info = nemo_creation_date_update_file_info;
    iface->cancel_update = nemo_creation_date_cancel_update;
}

static void
nemo_creation_date_nd_provider_iface_init (NemoNameAndDescProviderIface *iface)
{
    iface->get_name_and_desc = nemo_creation_date_get_name_and_desc;
}

static void
nemo_creation_date_class_init (NemoCreationDateClass *class)
{
    G_OBJECT_CLASS (class)->finalize = nemo_creation_date_finalize;
}

static void
nemo_creation_date_class_finalize (NemoCreationDateClass *class)
{
}

void
nemo_module_initialize (GTypeModule  *module)
{
    g_print ("Initializing nemo-creation-date extension\n");

    bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

    nemo_creation_date_register_type (module);
}

/* Perform module-specific shutdown. */
void
nemo_module_shutdown   (void)
{
    g_print ("Shutting down nemo-creation-date extension\n");
    /* FIXME freeing */
}

/* List all the extension types.  */
void 
nemo_module_list_types (const GType **types,
                        int          *num_types)
{
    static GType type_list[1];

    type_list[0] = NEMO_TYPE_CREATION_DATE;

    *types = type_list;
    *num_types = 1;
}

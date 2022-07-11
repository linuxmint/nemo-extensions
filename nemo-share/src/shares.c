#include <config.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <glib/gi18n-lib.h>
#include "shares.h"

#undef DEBUG_SHARES
#ifdef DEBUG_SHARES
#  define NET_USERSHARE_ARGV0 "debug-net-usershare"
#else
#  define NET_USERSHARE_ARGV0 "net"
#endif

static GHashTable *path_share_info_hash;
static GHashTable *share_name_share_info_hash;

#define NUM_CALLS_BETWEEN_TIMESTAMP_UPDATES 100
#define TIMESTAMP_THRESHOLD 10	/* seconds */
static int refresh_timestamp_update_counter;
static time_t refresh_timestamp;

#define KEY_PATH "path"
#define KEY_COMMENT "comment"
#define KEY_ACL "usershare_acl"
#define KEY_GUEST_OK "guest_ok"
#define GROUP_ALLOW_GUESTS "global"
#define KEY_ALLOW_GUESTS "usershare allow guests"

/* Debugging flags */
static gboolean throw_error_on_refresh;
static gboolean throw_error_on_add;
static gboolean throw_error_on_modify;
static gboolean throw_error_on_remove;



/* Interface to "net usershare" */

static gboolean
net_usershare_run (int argc, char **argv, GKeyFile **ret_key_file, GError **error)
{
	int real_argc;
	int i;
	char **real_argv;
	gboolean retval;
	char *stdout_contents;
	char *stderr_contents;
	int exit_status;
	int exit_code;
	GKeyFile *key_file;
	GError *real_error;

	g_assert (argc > 0);
	g_assert (argv != NULL);
	g_assert (error == NULL || *error == NULL);

	if (ret_key_file)
		*ret_key_file = NULL;

	/* Build command line */

	real_argc = 2 + argc + 1; /* "net" "usershare" [argv] NULL */
	real_argv = g_new (char *, real_argc);

	real_argv[0] = NET_USERSHARE_ARGV0;
	real_argv[1] = "usershare";

	for (i = 0; i < argc; i++) {
		g_assert (argv[i] != NULL);
		real_argv[i + 2] = argv[i];
	}

	real_argv[real_argc - 1] = NULL;

	/* Launch */

	stdout_contents = NULL;
	stderr_contents = NULL;
	/*
	{
		char **p;

		g_message ("------------------------------------------");

		for (p = real_argv; *p; p++)
			g_message ("spawn arg \"%s\"", *p);

		g_message ("end of spawn args; SPAWNING\n");
	}
	*/
	real_error = NULL;
	retval = g_spawn_sync (NULL,			/* cwd */
			       real_argv,
			       NULL, 			/* envp */
			       G_SPAWN_SEARCH_PATH,
			       NULL, 			/* GSpawnChildSetupFunc */
			       NULL,			/* user_data */
			       &stdout_contents,
			       &stderr_contents,
			       &exit_status,
			       &real_error);

	/* g_message ("returned from spawn: %s: %s", retval ? "SUCCESS" : "FAIL", retval ? "" : real_error->message); */

	if (!retval) {
		g_propagate_error (error, real_error);
		goto out;
	}

	if (!WIFEXITED (exit_status)) {
		g_message ("WIFEXITED(%d) was false!", exit_status);
		retval = FALSE;

		if (WIFSIGNALED (exit_status)) {
			int signal_num;

			signal_num = WTERMSIG (exit_status);
			g_message ("Child got signal %d", signal_num);

			g_set_error (error,
				     SHARES_ERROR,
				     SHARES_ERROR_FAILED,
				     _("%s %s %s returned with signal %d"),
				     real_argv[0],
				     real_argv[1],
				     real_argv[2],
				     signal_num);
		} else
			g_set_error (error,
				     SHARES_ERROR,
				     SHARES_ERROR_FAILED,
				     _("%s %s %s failed for an unknown reason"),
				     real_argv[0],
				     real_argv[1],
				     real_argv[2]);

		goto out;
	}

	exit_code = WEXITSTATUS (exit_status);

	/* g_message ("exit code %d", exit_code); */
	if (exit_code != 0) {
		char *str;
		char *message;

		/* stderr_contents is in the system locale encoding, not UTF-8 */

		str = g_locale_to_utf8 (stderr_contents, -1, NULL, NULL, NULL);

		if (str && str[0])
			message = g_strdup_printf (_("'net usershare' returned error %d: %s"), exit_code, str);
		else
			message = g_strdup_printf (_("'net usershare' returned error %d"), exit_code);

		g_free (str);

		g_set_error (error,
			     G_SPAWN_ERROR,
			     G_SPAWN_ERROR_FAILED,
			     "%s",
			     message);

		g_free (message);

		retval = FALSE;
		goto out;
	}

	if (ret_key_file) {
	  /* g_message ("caller wants GKeyFile"); */

		*ret_key_file = NULL;

		/* FIXME: jeallison@novell.com says the output of "net usershare" is nearly always
		 * in UTF-8, but that it can be configured in the master smb.conf.  We assume
		 * UTF-8 for now.
		 */

		if (!g_utf8_validate (stdout_contents, -1, NULL)) {
			g_message ("stdout of net usershare was not in valid UTF-8");
			g_set_error (error,
				     G_SPAWN_ERROR,
				     G_SPAWN_ERROR_FAILED,
				     _("the output of 'net usershare' is not in valid UTF-8 encoding"));
			retval = FALSE;
			goto out;
		}

		key_file = g_key_file_new ();

		real_error = NULL;
		if (!g_key_file_load_from_data (key_file, stdout_contents, -1, 0, &real_error)) {
			g_message ("Error when parsing key file {\n%s\n}: %s", stdout_contents, real_error->message);
			g_propagate_error (error, real_error);
			g_key_file_free (key_file);
			retval = FALSE;
			goto out;
		}

		retval = TRUE;
		*ret_key_file = key_file;
	} else
		retval = TRUE;

	/* g_message ("success from calling net usershare and parsing its output"); */

 out:
	g_free (real_argv);
	g_free (stdout_contents);
	g_free (stderr_contents);

	/* g_message ("------------------------------------------"); */

	return retval;
}



/* Internals */

static void
ensure_hashes (void)
{
	if (path_share_info_hash == NULL) {
		g_assert (share_name_share_info_hash == NULL);

		path_share_info_hash = g_hash_table_new (g_str_hash, g_str_equal);
		share_name_share_info_hash = g_hash_table_new (g_str_hash, g_str_equal);
	} else
		g_assert (share_name_share_info_hash != NULL);
}

static ShareInfo *
lookup_share_by_path (const char *path)
{
	ensure_hashes ();
	return g_hash_table_lookup (path_share_info_hash, path);
}

static ShareInfo *
lookup_share_by_share_name (const char *share_name)
{
	ensure_hashes ();
	return g_hash_table_lookup (share_name_share_info_hash, share_name);
}

static void
add_share_info_to_hashes (ShareInfo *info)
{
	ensure_hashes ();
	g_hash_table_insert (path_share_info_hash, info->path, info);
	g_hash_table_insert (share_name_share_info_hash, info->share_name, info);
}

static void
remove_share_info_from_hashes (ShareInfo *info)
{
	ensure_hashes ();
	g_hash_table_remove (path_share_info_hash, info->path);
	g_hash_table_remove (share_name_share_info_hash, info->share_name);
}

static gboolean
remove_from_path_hash_cb (gpointer key,
			  gpointer value,
			  gpointer data)
{
	ShareInfo *info;

	info = value;
	shares_free_share_info (info);

	return TRUE;
}

static gboolean
remove_from_share_name_hash_cb (gpointer key,
				gpointer value,
				gpointer data)
{
	/* The ShareInfo was already freed in remove_from_path_hash_cb() */
	return TRUE;
}

static void
free_all_shares (void)
{
	ensure_hashes ();
	g_hash_table_foreach_remove (path_share_info_hash, remove_from_path_hash_cb, NULL);
	g_hash_table_foreach_remove (share_name_share_info_hash, remove_from_share_name_hash_cb, NULL);
}

static char *
get_string_from_key_file (GKeyFile *key_file, const char *group, const char *key)
{
	GError *error;
	char *str;

	error = NULL;
	str = NULL;

	if (g_key_file_has_key (key_file, group, key, &error)) {
		str = g_key_file_get_string (key_file, group, key, &error);
		if (!str) {
			g_assert (!g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_NOT_FOUND)
				  && !g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND));

			g_error_free (error);
		}
	} else {
		g_assert (!g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND));
		g_error_free (error);
	}

	return str;
}

static void
add_key_group_to_hashes (GKeyFile *key_file, const char *group)
{
	char *path;
	char *comment;
	char *acl;
	gboolean is_writable;
	char *guest_ok_str;
	gboolean guest_ok;
	ShareInfo *info;
	ShareInfo *old_info;

	/* Remove the old share based on the name */

	old_info = lookup_share_by_share_name (group);
	if (old_info) {
		remove_share_info_from_hashes (old_info);
		shares_free_share_info (old_info);
	}

	/* Start parsing, and remove the old share based on the path */

	path = get_string_from_key_file (key_file, group, KEY_PATH);
	if (!path) {
		g_message ("group '%s' doesn't have a '%s' key!  Ignoring group.", group, KEY_PATH);
		return;
	}

	old_info = lookup_share_by_path (path);
	if (old_info) {
		remove_share_info_from_hashes (old_info);
		shares_free_share_info (old_info);
	}

	/* Finish parsing */

	comment = get_string_from_key_file (key_file, group, KEY_COMMENT);

	acl = get_string_from_key_file (key_file, group, KEY_ACL);
	if (acl) {
		if (strstr (acl, "Everyone:R"))
			is_writable = FALSE;
		else if (strstr (acl, "Everyone:F"))
			is_writable = TRUE;
		else {
			g_message ("unknown format for key '%s/%s' as it contains '%s'.  Assuming that the share is read-only",
				   group, KEY_ACL, acl);
			is_writable = FALSE;
		}

		g_free (acl);
	} else {
		g_message ("group '%s' doesn't have a '%s' key!  Assuming that the share is read-only.", group, KEY_ACL);
		is_writable = FALSE;
	}

	guest_ok_str = get_string_from_key_file (key_file, group, KEY_GUEST_OK);
	if (guest_ok_str) {
		if (strcmp (guest_ok_str, "n") == 0)
			guest_ok = FALSE;
		else if (strcmp (guest_ok_str, "y") == 0)
			guest_ok = TRUE;
		else {
			g_message ("unknown format for key '%s/%s' as it contains '%s'.  Assuming that the share is not guest accessible.",
				   group, KEY_GUEST_OK, guest_ok_str);
			guest_ok = FALSE;
		}

		g_free (guest_ok_str);
	} else {
		g_message ("group '%s' doesn't have a '%s' key!  Assuming that the share is not guest accessible.", group, KEY_GUEST_OK);
		guest_ok = FALSE;
	}

	g_assert (path != NULL);
	g_assert (group != NULL);

	info = g_new (ShareInfo, 1);
	info->path = path;
	info->share_name = g_strdup (group);
	info->comment = comment;
	info->is_writable = is_writable;
	info->guest_ok = guest_ok;

	add_share_info_to_hashes (info);
}

static void
replace_shares_from_key_file (GKeyFile *key_file)
{
	gsize num_groups;
	char **group_names;
	gsize i;

	group_names = g_key_file_get_groups (key_file, &num_groups);

	/* FIXME: In add_key_group_to_hashes(), we simply ignore key groups
	 * which have invalid data (i.e. no path).  We could probably accumulate a
	 * GError with the list of invalid groups and propagate it upwards.
	 */
	for (i = 0; i < num_groups; i++) {
		g_assert (group_names[i] != NULL);
		add_key_group_to_hashes (key_file, group_names[i]);
	}

	g_strfreev (group_names);
}

static gboolean
refresh_shares (GError **error)
{
	GKeyFile *key_file;
	char *argv[1];
	GError *real_error;

	free_all_shares ();

	if (throw_error_on_refresh) {
		g_set_error (error,
			     SHARES_ERROR,
			     SHARES_ERROR_FAILED,
			     _("Failed"));
		return FALSE;
	}

	argv[0] = "info";

	real_error = NULL;
	if (!net_usershare_run (G_N_ELEMENTS (argv), argv, &key_file, &real_error)) {
		g_message ("Called \"net usershare info\" but it failed: %s", real_error->message);
		g_propagate_error (error, real_error);
		return FALSE;
	}

	g_assert (key_file != NULL);

	replace_shares_from_key_file (key_file);
	g_key_file_free (key_file);

	return TRUE;
}

static gboolean
refresh_if_needed (GError **error)
{
	gboolean retval;

	if (refresh_timestamp_update_counter == 0) {
		time_t new_timestamp;

		refresh_timestamp_update_counter = NUM_CALLS_BETWEEN_TIMESTAMP_UPDATES;

		new_timestamp = time (NULL);
		if (new_timestamp - refresh_timestamp > TIMESTAMP_THRESHOLD) {
		  /* g_message ("REFRESHING SHARES"); */
			retval = refresh_shares (error);
		} else
			retval = TRUE;

		refresh_timestamp = new_timestamp;
	} else {
		refresh_timestamp_update_counter--;
		retval = TRUE;
	}

	return retval;
}

static ShareInfo *
copy_share_info (ShareInfo *info)
{
	ShareInfo *copy;

	if (!info)
		return NULL;

	copy = g_new (ShareInfo, 1);
	copy->path = g_strdup (info->path);
	copy->share_name = g_strdup (info->share_name);
	copy->comment = g_strdup (info->comment);
	copy->is_writable = info->is_writable;
	copy->guest_ok = info->guest_ok;

	return copy;
}

/**
 * shares_supports_guest_ok:
 * @supports_guest_ok_ret: Location to store whether "usershare allow guests"
 * is enabled.
 * @error: Location to store error, or #NULL.
 *
 * Determines whether the option "usershare allow guests" is enabled in samba
 * config as shown by testparm.
 *
 * Return value: #TRUE if if the info could be queried successfully, #FALSE
 * otherwise.  If this function returns #FALSE, an error code will be returned
 * in the @error argument, and *@ret_info_list will be set to #FALSE.
 **/
gboolean
shares_supports_guest_ok (gboolean *supports_guest_ok_ret, GError **error)
{
	gboolean retval;
	gboolean result;
	char *stdout_contents;
	char *stderr_contents;
	int exit_status;
	int exit_code;

	*supports_guest_ok_ret = FALSE;

	result = g_spawn_command_line_sync ("testparm -s --parameter-name='usershare allow guests'",
					    &stdout_contents,
					    &stderr_contents,
					    &exit_status,
					    error);
	if (!result)
		return FALSE;

	retval = FALSE;

	if (!WIFEXITED (exit_status)) {
		if (WIFSIGNALED (exit_status)) {
			int signal_num;

			signal_num = WTERMSIG (exit_status);
			g_set_error (error,
				     SHARES_ERROR,
				     SHARES_ERROR_FAILED,
				     _("Samba's testparm returned with signal %d"),
				     signal_num);
		} else
			g_set_error (error,
				     SHARES_ERROR,
				     SHARES_ERROR_FAILED,
				     _("Samba's testparm failed for an unknown reason"));

		goto out;
	}

	exit_code = WEXITSTATUS (exit_status);
	if (exit_code != 0) {
		char *str;
		char *message;

		/* stderr_contents is in the system locale encoding, not UTF-8 */

		str = g_locale_to_utf8 (stderr_contents, -1, NULL, NULL, NULL);

		if (str && str[0])
			message = g_strdup_printf (_("Samba's testparm returned error %d: %s"), exit_code, str);
		else
			message = g_strdup_printf (_("Samba's testparm returned error %d"), exit_code);

		g_free (str);

		g_set_error (error,
			     G_SPAWN_ERROR,
			     G_SPAWN_ERROR_FAILED,
			     "%s",
			     message);

		g_free (message);

		goto out;
	}

	retval = TRUE;
	*supports_guest_ok_ret = (g_ascii_strncasecmp (stdout_contents, "Yes", 3) == 0);

 out:
	g_free (stdout_contents);
	g_free (stderr_contents);

	return retval;
}

static gboolean
add_share (ShareInfo *info, GError **error)
{
	char *argv[7];
	int argc;
	ShareInfo *copy;
	GKeyFile *key_file;
	GError *real_error;
	gboolean supports_success;
	gboolean supports_guest_ok;
	gboolean net_usershare_success;

	/*	g_message ("add_share() start"); */

	if (throw_error_on_add) {
		g_set_error (error,
			     SHARES_ERROR,
			     SHARES_ERROR_FAILED,
			     _("Failed"));
		g_message ("add_share() end FAIL");
		return FALSE;
	}

	supports_success = shares_supports_guest_ok (&supports_guest_ok, error);
	if (!supports_success)
		return FALSE;

	argv[0] = "add";
	argv[1] = "--long";
	argv[2] = info->share_name;
	argv[3] = info->path;
	argv[4] = info->comment;
	argv[5] = info->is_writable ? "Everyone:F" : g_strdup_printf ("Everyone:R,%s:F", g_get_user_name ());

	if (supports_guest_ok) {
		argv[6] = info->guest_ok ? "guest_ok=y" : "guest_ok=n";
		argc = 7;
	} else
		argc = 6;

	real_error = NULL;
	net_usershare_success = net_usershare_run (argc, argv, &key_file, &real_error);
	if (!info->is_writable) g_free (argv[5]);

	if (!net_usershare_success) {
		g_message ("Called \"net usershare add\" but it failed: %s", real_error->message);
		g_propagate_error (error, real_error);
		return FALSE;
	}

	replace_shares_from_key_file (key_file);

	copy = copy_share_info (info);
	add_share_info_to_hashes (copy);

	/* g_message ("add_share() end SUCCESS"); */

	return TRUE;
}

static gboolean
remove_share (const char *path, GError **error)
{
	ShareInfo *old_info;
	char *argv[2];
	GError *real_error;

	/* g_message ("remove_share() start"); */

	if (throw_error_on_remove) {
		g_set_error (error,
			     SHARES_ERROR,
			     SHARES_ERROR_FAILED,
			     "Failed");
		g_message ("remove_share() end FAIL");
		return FALSE;
	}

	old_info = lookup_share_by_path (path);
	if (!old_info) {
		char *display_name;

		display_name = g_filename_display_name (path);
		g_set_error (error,
			     SHARES_ERROR,
			     SHARES_ERROR_NONEXISTENT,
			     _("Cannot remove the share for path %s: that path is not shared"),
			     display_name);
		g_free (display_name);

		g_message ("remove_share() end FAIL: path %s was not in our hashes", path);
		return FALSE;
	}

	argv[0] = "delete";
	argv[1] = old_info->share_name;

	real_error = NULL;
	if (!net_usershare_run (G_N_ELEMENTS (argv), argv, NULL, &real_error)) {
		g_message ("Called \"net usershare delete\" but it failed: %s", real_error->message);
		g_propagate_error (error, real_error);
		g_message ("remove_share() end FAIL");
		return FALSE;
	}

	remove_share_info_from_hashes (old_info);
	shares_free_share_info (old_info);

	/* g_message ("remove_share() end SUCCESS"); */

	return TRUE;
}

static gboolean
modify_share (const char *old_path, ShareInfo *info, GError **error)
{
	ShareInfo *old_info;

 	/* g_message ("modify_share() start"); */

	old_info = lookup_share_by_path (old_path);
	if (old_info == NULL) {
	  /*g_message ("modify_share() end; calling add_share() instead");*/
		return add_share (info, error);
	}

	g_assert (old_info != NULL);

	if (strcmp (info->path, old_info->path) != 0) {
		g_set_error (error,
			     SHARES_ERROR,
			     SHARES_ERROR_FAILED,
			     _("Cannot change the path of an existing share; please remove the old share first and add a new one"));
		g_message ("modify_share() end FAIL: tried to change the path in a share!");
		return FALSE;
	}

	if (throw_error_on_modify) {
		g_set_error (error,
			     SHARES_ERROR,
			     SHARES_ERROR_FAILED,
			     "Failed");
		g_message ("modify_share() end FAIL");
		return FALSE;
	}

	/* Although "net usershare add" will modify an existing share if it has the same share name
	 * as the one that gets passed in, our semantics are different.  We have a one-to-one mapping
	 * between paths and share names; "net usershare" supports a one-to-many mapping from paths
	 * to share names.  So, we must first remove the old share and then add the new/modified one.
	 */

	if (!remove_share (old_path, error)) {
		g_message ("modify_share() end FAIL: error when removing old share");
		return FALSE;
	}

	/* g_message ("modify_share() end: will call add_share() with the new share info"); */
	return add_share (info, error);
}



/* Public API */

GQuark
shares_error_quark (void)
{
	static GQuark quark;

	if (quark == 0)
		quark = g_quark_from_string ("nemo-shares-error-quark"); /* not from_static_string since we are a module */

	return quark;
}

/**
 * shares_free_share_info:
 * @info: A #ShareInfo structure.
 *
 * Frees a #ShareInfo structure.
 **/
void
shares_free_share_info (ShareInfo *info)
{
	g_assert (info != NULL);

	g_free (info->path);
	g_free (info->share_name);
	g_free (info->comment);
	g_free (info);
}

/**
 * shares_get_path_is_shared:
 * @path: A full path name ("/foo/bar/baz") in file system encoding.
 * @ret_is_shared: Location to store result value (#TRUE if the path is shared, #FALSE otherwise)
 * @error: Location to store error, or #NULL.
 *
 * Checks whether a path is shared through Samba.
 *
 * Return value: #TRUE if the info could be queried successfully, #FALSE
 * otherwise.  If this function returns #FALSE, an error code will be returned in the
 * @error argument, and *@ret_is_shared will be set to #FALSE.
 **/
gboolean
shares_get_path_is_shared (const char *path, gboolean *ret_is_shared, GError **error)
{
	g_assert (ret_is_shared != NULL);
	g_assert (error == NULL || *error == NULL);

	if (!refresh_if_needed (error)) {
		*ret_is_shared = FALSE;
		return FALSE;
	}

	*ret_is_shared = (lookup_share_by_path (path) != NULL);

	return TRUE;
}

/**
 * shares_get_share_info_for_path:
 * @path: A full path name ("/foo/bar/baz") in file system encoding.
 * @ret_share_info: Location to store result with the share's info - on return,
 * will be non-NULL if the path is indeed shared, or #NULL if the path is not
 * shared.  You must free the non-NULL value with shares_free_share_info().
 * @error: Location to store error, or #NULL.
 *
 * Queries the information for a shared path:  its share name, its read-only status, etc.
 *
 * Return value: #TRUE if the info could be queried successfully, #FALSE
 * otherwise.  If this function returns #FALSE, an error code will be returned in the
 * @error argument, and *@ret_share_info will be set to #NULL.
 **/
gboolean
shares_get_share_info_for_path (const char *path, ShareInfo **ret_share_info, GError **error)
{
	ShareInfo *info;

	g_assert (path != NULL);
	g_assert (ret_share_info != NULL);
	g_assert (error == NULL || *error == NULL);

	if (!refresh_if_needed (error)) {
		*ret_share_info = NULL;
		return FALSE;
	}

	info = lookup_share_by_path (path);
	*ret_share_info = copy_share_info (info);

	return TRUE;
}

/**
 * shares_get_share_name_exists:
 * @share_name: Name of a share.
 * @ret_exists: Location to store return value; #TRUE if the share name exists, #FALSE otherwise.
 *
 * Queries whether a share name already exists in the user's list of shares.
 *
 * Return value: #TRUE if the info could be queried successfully, #FALSE
 * otherwise.  If this function returns #FALSE, an error code will be returned in the
 * @error argument, and *@ret_exists will be set to #FALSE.
 **/
gboolean
shares_get_share_name_exists (const char *share_name, gboolean *ret_exists, GError **error)
{
	g_assert (share_name != NULL);
	g_assert (ret_exists != NULL);
	g_assert (error == NULL || *error == NULL);

	if (!refresh_if_needed (error)) {
		*ret_exists = FALSE;
		return FALSE;
	}

	*ret_exists = (lookup_share_by_share_name (share_name) != NULL);

	return TRUE;
}

/**
 * shares_get_share_info_for_share_name:
 * @share_name: Name of a share.
 * @ret_share_info: Location to store result with the share's info - on return,
 * will be non-NULL if there is a share for the specified name, or #NULL if no
 * share has such name.  You must free the non-NULL value with
 * shares_free_share_info().
 * @error: Location to store error, or #NULL.
 *
 * Queries the information for the share which has a specific name.
 *
 * Return value: #TRUE if the info could be queried successfully, #FALSE
 * otherwise.  If this function returns #FALSE, an error code will be returned in the
 * @error argument, and *@ret_share_info will be set to #NULL.
 **/
gboolean
shares_get_share_info_for_share_name (const char *share_name, ShareInfo **ret_share_info, GError **error)
{
	ShareInfo *info;

	g_assert (share_name != NULL);
	g_assert (ret_share_info != NULL);
	g_assert (error == NULL || *error == NULL);

	if (!refresh_if_needed (error)) {
		*ret_share_info = NULL;
		return FALSE;
	}

	info = lookup_share_by_share_name (share_name);
	*ret_share_info = copy_share_info (info);

	return TRUE;
}

/**
 * shares_modify_share:
 * @old_path: Path of the share to modify, or %NULL.
 * @info: Info of the share to modify/add, or %NULL to delete a share.
 * @error: Location to store error, or #NULL.
 *
 * Can add, modify, or delete shares.  To add a share, pass %NULL for @old_path,
 * and a non-null @info.  To modify a share, pass a non-null @old_path and
 * non-null @info; in this case, @info->path must have the same contents as
 * @old_path.  To remove a share, pass a non-NULL @old_path and a %NULL @info.
 *
 * Return value: TRUE if the share could be modified, FALSE otherwise.  If this returns
 * FALSE, then the error information will be placed in @error.
 **/
gboolean
shares_modify_share (const char *old_path, ShareInfo *info, GError **error)
{
	g_assert ((old_path == NULL && info != NULL)
		  || (old_path != NULL && info == NULL)
		  || (old_path != NULL && info != NULL));
	g_assert (error == NULL || *error == NULL);

	if (!refresh_if_needed (error))
		return FALSE;

	if (old_path == NULL)
		return add_share (info, error);
	else if (info == NULL)
		return remove_share (old_path, error);
	else
		return modify_share (old_path, info, error);
}

static void
copy_to_slist_cb (gpointer key, gpointer value, gpointer data)
{
	ShareInfo *info;
	ShareInfo *copy;
	GSList **list;

	info = value;
	list = data;

	copy = copy_share_info (info);
	*list = g_slist_prepend (*list, copy);
}

/**
 * shares_get_share_info_list:
 * @ret_info_list: Location to store the return value, which is a list
 * of #ShareInfo structures.  Free this with shares_free_share_info_list().
 * @error: Location to store error, or #NULL.
 *
 * Gets the list of shared folders and their information.
 *
 * Return value: #TRUE if the info could be queried successfully, #FALSE
 * otherwise.  If this function returns #FALSE, an error code will be returned in the
 * @error argument, and *@ret_info_list will be set to #NULL.
 **/
gboolean
shares_get_share_info_list (GSList **ret_info_list, GError **error)
{
	g_assert (ret_info_list != NULL);
	g_assert (error == NULL || *error == NULL);

	if (!refresh_if_needed (error)) {
		*ret_info_list = NULL;
		return FALSE;
	}

	*ret_info_list = NULL;
	g_hash_table_foreach (path_share_info_hash, copy_to_slist_cb, ret_info_list);

	return TRUE;
}

/**
 * shares_free_share_info_list:
 * @list: List of #ShareInfo structures, or %NULL.
 *
 * Frees a list of #ShareInfo structures as returned by shares_get_share_info_list().
 **/
void
shares_free_share_info_list (GSList *list)
{
	GSList *l;

	for (l = list; l; l = l->next) {
		ShareInfo *info;

		info = l->data;
		shares_free_share_info (l->data);
	}

	g_slist_free (list);
}

void
shares_set_debug (gboolean error_on_refresh,
		  gboolean error_on_add,
		  gboolean error_on_modify,
		  gboolean error_on_remove)
{
	throw_error_on_refresh = error_on_refresh;
	throw_error_on_add = error_on_add;
	throw_error_on_modify = error_on_modify;
	throw_error_on_remove = error_on_remove;
}

/* main.c
 *
 * Copyright 2022 Patrick Winters
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written
 * authorization.
 */

#include "replit-config.h"

#include <glib.h>
#include <stdlib.h>

#define CLI_NAME "rquery"
#define CLI_SUMM "A command-line tool for interacting with Replit."

gint main(gint argc, gchar* argv[]) {
	gboolean version = FALSE;
	gboolean subscribe = FALSE;
	gchar* variables = NULL;
	gchar* token = NULL;
	gchar* query_file = NULL;
	gchar** query = NULL;

	GOptionEntry main_entries[] = {
		{ "version", 'v', 0, G_OPTION_ARG_NONE, &version, "Show program version" },
		{ "subscribe", 's', 0, G_OPTION_ARG_NONE, &subscribe, "Create subscription" },
		{ "token", 't', 0, G_OPTION_ARG_STRING, &token, "Set connect.sid cookie" },
		{ "variables", 'j', 0, G_OPTION_ARG_STRING, &variables, "Include variables" },
		{ "query", 'f', 0, G_OPTION_ARG_FILENAME, &query_file, "Read query from file" },
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &query },
		{ NULL }
	};

	g_autoptr(GOptionContext) context = g_option_context_new("[QUERY]");
	g_option_context_add_main_entries(context, main_entries, NULL);
	g_option_context_set_summary(context, CLI_SUMM);

	g_autoptr(GError) error = NULL;

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_printerr("%s\n", error->message);
		return EXIT_FAILURE;
	}

	if (version) {
		if (subscribe || variables || token || query_file || query) {
			g_printerr("%s\n", "Cannot specify --version with other options");
			return EXIT_FAILURE;
		}

		g_printerr("%s %s\n", CLI_NAME, PACKAGE_VERSION);
		return EXIT_SUCCESS;
	}

	// todo: functionality

	return EXIT_SUCCESS;
}

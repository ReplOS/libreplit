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
#include <json-glib/json-glib.h>
#include <replit.h>
#include <stdlib.h>

#define CLI_NAME "rquery"
#define CLI_SUMM "A command-line tool for interacting with Replit."

void output_response(JsonNode* res);

gint main(gint argc, gchar* argv[]) {
	gboolean version = FALSE;
	gboolean subscribe = FALSE;
	const gchar* variables = NULL;
	const gchar* token = NULL;
	const gchar* query_file = NULL;
	const gchar** query_strings = NULL;

	GOptionEntry main_entries[] = {
		{ "version", 'v', 0, G_OPTION_ARG_NONE, &version, "Show program version" },
		{ "subscribe", 's', 0, G_OPTION_ARG_NONE, &subscribe, "Create subscription" },
		{ "token", 't', 0, G_OPTION_ARG_STRING, &token, "Set connect.sid cookie" },
		{ "variables", 'r', 0, G_OPTION_ARG_STRING, &variables, "Include variables" },
		{ "query", 'f', 0, G_OPTION_ARG_FILENAME, &query_file, "Read query from file" },
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &query_strings },
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
		if (subscribe || variables || token || query_file || query_strings) {
			g_printerr("%s\n", "Cannot specify --version with other options");
			return EXIT_FAILURE;
		}

		g_printerr("%s %s\n", CLI_NAME, PACKAGE_VERSION);
		return EXIT_SUCCESS;
	}

	if (token == NULL) {
		do {
			token = g_getenv("REPLIT_TOKEN");
			if (token != NULL) break;

			token = g_getenv("CONNECT_SID");
			if (token != NULL) break;

			g_printerr("%s\n", "No token provided and no variable found");
			return EXIT_FAILURE;
		} while (0);
	}

	gchar* query;

	if (query_strings != NULL) {
		if (query_strings[1] != NULL) {
			g_printerr("%s\n", "At most one query may be specified");
			return EXIT_FAILURE;
		}

		query = g_strdup(query_strings[0]);
	} else if (query_file != NULL) {
		if (!g_file_get_contents(query_file, &query, NULL, &error)) {
			g_printerr("%s\n", error->message);
			return EXIT_FAILURE;
		}
	} else {
		g_printerr("%s\n", "At least one query must be specified");
		return EXIT_FAILURE;
	}

	JsonNode* variables_json = NULL;

	if (variables != NULL) {
		JsonParser* parser = json_parser_new();

		if (!json_parser_load_from_data(parser, variables, -1, &error)) {
			g_printerr("%s\n", error->message);
			return EXIT_FAILURE;
		}

		variables_json = json_parser_get_root(parser);
	}

	ReplitClient* client = replit_client_new(token);

	if (subscribe) {
		//
	} else {
		JsonNode* res = replit_client_query(client, query, variables_json, &error);

		if (res == NULL) {
			g_printerr("%s\n", error->message);
			return EXIT_FAILURE;
		} else {
			output_response(res);
		}
	}

	return EXIT_SUCCESS;
}

void output_response(JsonNode* res) {
	// TODO
}

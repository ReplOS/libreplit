/* replit-client.h
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

#pragma once

#if !defined(REPLIT_INSIDE) && !defined(REPLIT_COMPILATION)
#error "Only <replit.h> can be included directly."
#endif

#include <glib.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

/**
 * REPLIT_DOMAIN:
 * 
 * The host name connected to by #ReplitClient.
 * 
 * It may be useful to library users performing other requests to Replit. This
 * is not configurable at runtime.
 */
#define REPLIT_DOMAIN "replit.com"

/**
 * REPLIT_HC_KEY:
 * 
 * Replit's unique hCaptcha site key.
 * 
 * This may be useful to library users needing to present a hCaptcha CAPTCHA to
 * their human end-users for authentication purposes. It should only be directly
 * useful to the hCaptcha client library.
 */
#define REPLIT_HC_KEY "473079ba-e99f-4e25-a635-e9b661c7dd3e"

/**
 * replit_client_error_quark: (skip)
 * 
 * Creates and returns the quark for #ReplitClientError.
 * 
 * Returns: The created quark.
 */
GQuark replit_client_error_quark(void);
#define REPLIT_CLIENT_ERROR replit_client_error_quark()

/**
 * ReplitClientError:
 * 
 * Error codes to be returned by #ReplitClient methods.
 */
typedef enum {
	/**
	 * REPLIT_CLIENT_ERROR_RESPONSE_STATUS:
	 * 
	 * Replit returned an error (non-200) in the response status code.
	 */
	REPLIT_CLIENT_ERROR_RESPONSE_STATUS,

	/**
	 * REPLIT_CLIENT_ERROR_LOGIN_FAILED:
	 * 
	 * Replit refused to log in with the provided credentials.
	 */
	REPLIT_CLIENT_ERROR_LOGIN_FAILED,

	/**
	 * REPLIT_CLIENT_ERROR_GRAPHQL_ERROR:
	 * 
	 * Replit returned a GraphQL response containing at least one error.
	 */
	REPLIT_CLIENT_ERROR_GRAPHQL_ERROR,

	/**
	 * REPLIT_CLIENT_ERROR_GRAPHQL_EMPTY:
	 * 
	 * Replit returned a GraphQL response containing no data.
	 */
	REPLIT_CLIENT_ERROR_GRAPHQL_EMPTY,
} ReplitClientError;

#define REPLIT_TYPE_CLIENT replit_client_get_type()
G_DECLARE_FINAL_TYPE (ReplitClient, replit_client, REPLIT, CLIENT, GObject)

ReplitClient *replit_client_new(const gchar *token);

JsonNode *replit_client_query(
	ReplitClient *client,
	const gchar *query,
	JsonNode *variables,
	GError **error
);

GObject *replit_client_query_to_object(
	ReplitClient *client,
	const gchar *query,
	JsonNode *variables,
	GType gtype,
	GError **error
);

gchar *replit_client_login(
	const gchar *username,
	const gchar *password,
	const gchar *captcha,
	GError **error
);

G_END_DECLS

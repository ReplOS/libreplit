/* replit-client.c
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

#include <libsoup/soup.h>

#include "replit-client.h"
#include "replit-version.h"

#define TOKEN_COOKIE "connect.sid"

G_DEFINE_QUARK (REPLIT_CLIENT_ERROR, replit_client_error)

/**
 * ReplitClient:
 * 
 * Represents a logged-in session for making API queries to Replit.
 * 
 * A #ReplitClient can be created with [ctor@Client.new], which requires passing
 * a token. This token corresponds to Replit's `connect.sid` cookie, and can be
 * obtained either manually, or by using [func@Client.login] to create one by
 * simulating a login with a username and password.
 * 
 * Whilst the inner properties may be accessible, it is advised against using
 * them. All regular usage should be possible through the provided public API.
 */

struct _ReplitClient {
	GObject parent_instance;

	gchar* token;
	SoupSession* session;
	SoupCookieJar* jar;
	ReplitSubscriber* subscriber;
};

G_DEFINE_TYPE (ReplitClient, replit_client, G_TYPE_OBJECT)

static void replit_client_dispose(GObject* gobject);
static void replit_client_finalize(GObject* gobject);

static void replit_client_class_init(ReplitClientClass* klass) {
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = replit_client_dispose;
	object_class->finalize = replit_client_finalize;
}

static void replit_client_init(ReplitClient* self __attribute__((unused))) {}

static void replit_client_dispose(GObject* gobject) {
	ReplitClient* self = REPLIT_CLIENT (gobject);

	g_clear_object(&self->session);
	g_clear_object(&self->jar);
	g_clear_object(&self->subscriber);

	G_OBJECT_CLASS (replit_client_parent_class)->dispose(gobject);
}

static void replit_client_finalize(GObject* gobject) {
	ReplitClient* self = REPLIT_CLIENT (gobject);

	g_free(self->token);

	G_OBJECT_CLASS (replit_client_parent_class)->finalize(gobject);
}

/**
 * replit_client_new:
 * @token: (transfer none): The token to use as the `connect.sid` cookie.
 * 
 * Creates a new #ReplitClient with the given login token.
 * 
 * Returns: (transfer full): The new #ReplitClient.
 */
ReplitClient* replit_client_new(const gchar* token) {
	SoupSession* session = soup_session_new();

	SoupCookie* cookie = soup_cookie_new(TOKEN_COOKIE, token, REPLIT_DOMAIN, "/", -1);

	SoupCookieJar* jar = soup_cookie_jar_new();
	soup_cookie_jar_add_cookie(jar, cookie);

	soup_session_add_feature(session, SOUP_SESSION_FEATURE (jar));

	ReplitClient* self = g_object_new(REPLIT_TYPE_CLIENT, NULL);
	self->token = g_strdup(token);
	self->session = session;
	self->jar = jar;

	return self;
}

/**
 * replit_client_query:
 * @client: The client.
 * @query: (transfer none): The GraphQL query or mutation to perform.
 * @variables: (transfer full) (nullable): Any variables to pass with the query.
 * 
 * Sends a GraphQL query or mutation to Replit to perform as the current user.
 * 
 * If @variables is %NULL, an empty object will be sent in its place. Otherwise,
 * it should usually be an object containing any variables used by the query.
 * 
 * Returns: (transfer full) (nullable): The returned data, or %NULL on error.
 */
JsonNode* replit_client_query(
	ReplitClient* self,
	const gchar* query,
	JsonNode* variables,
	GError** error
) {
	if (variables == NULL) {
		variables = json_node_new(JSON_NODE_OBJECT);
		json_node_set_object(variables, json_object_new());
	}

	JsonBuilder* builder = json_builder_new();
	json_builder_begin_object(builder);
	json_builder_set_member_name(builder, "operationName");
	json_builder_add_null_value(builder);
	json_builder_set_member_name(builder, "query");
	json_builder_add_string_value(builder, query);
	json_builder_set_member_name(builder, "variables");
	json_builder_add_value(builder, variables);
	json_builder_end_object(builder);

	JsonGenerator* generator = json_generator_new();
	JsonNode* builder_root = json_builder_get_root(builder);
	json_generator_set_root(generator, builder_root);

	gsize req_length;
	gchar* req_body = json_generator_to_data(generator, &req_length);

	json_node_unref(builder_root);
	g_object_unref(builder);
	g_object_unref(generator);

	GBytes* req_bytes = g_bytes_new(req_body, req_length);

	g_free(req_body);

	GUri* uri = g_uri_build(0, "https", NULL, REPLIT_DOMAIN, -1, "/graphql", NULL, NULL);
	SoupMessage* msg = soup_message_new_from_uri(SOUP_METHOD_POST, uri);
	soup_message_set_request_body_from_bytes(msg, "application/json", req_bytes);

	SoupMessageHeaders* headers = soup_message_get_request_headers(msg);
	soup_message_headers_append(headers, "Referrer", "https://replit.com/");
	soup_message_headers_append(headers, "X-Requested-With", "XMLHttpRequest");
	soup_message_headers_append(headers, "X-Libreplit-Version", REPLIT_VERSION_S);

	GInputStream* stream = soup_session_send(self->session, msg, NULL, error);

	SoupStatus status = soup_message_get_status(msg);

	g_uri_unref(uri);
	g_object_unref(msg);

	if (stream == NULL) return NULL;

	if (status != SOUP_STATUS_OK) {
		g_set_error(
			error,
			REPLIT_CLIENT_ERROR,
			REPLIT_CLIENT_ERROR_RESPONSE_STATUS,
			"Server responded with status %d",
			status
		);

		return NULL;
	}

	JsonParser* parser = json_parser_new_immutable();
	gboolean ok = json_parser_load_from_stream(parser, stream, NULL, error);

	g_object_unref(stream);

	if (!ok) {
		g_object_unref(parser);

		return NULL;
	}

	JsonNode* root = json_parser_steal_root(parser);
	JsonObject* root_object = json_node_get_object(root);
	
	g_object_unref(parser);

	JsonNode* error_node = json_object_get_member(root_object, "error");

	if (error_node != NULL) {
		switch (json_node_get_node_type(error_node)) {
			case JSON_NODE_OBJECT:
				JsonObject* error_object = json_node_get_object(error_node);

				if (json_object_has_member(error_object, "errors")) {
					JsonArray* errors = json_object_get_array_member(error_object, "errors");
					guint errors_length = json_array_get_length(errors);

					if (errors_length > 0) {
						GString* error_object_buffer = g_string_new("");

						for (guint i = 0; i < errors_length; i++) {
							g_string_append(error_object_buffer, ", ");

							JsonObject* errors_element = json_array_get_object_element(errors, i);
							const gchar* errors_element_message =
								json_object_get_string_member(errors_element, "message");

							g_string_append(error_object_buffer, errors_element_message);
						}
						
						g_set_error_literal(
							error,
							REPLIT_CLIENT_ERROR,
							REPLIT_CLIENT_ERROR_GRAPHQL_ERROR,
							error_object_buffer->str + 2
						);

						g_string_free(error_object_buffer, TRUE);
				
						break;
					}
				}

				__attribute__ ((fallthrough));

			case JSON_NODE_VALUE:
				const gchar* error_string = json_node_get_string(error_node);

				if (error_string != NULL) {
					g_set_error_literal(
						error,
						REPLIT_CLIENT_ERROR,
						REPLIT_CLIENT_ERROR_GRAPHQL_ERROR,
						error_string
					);

					break;
				}

				__attribute__ ((fallthrough));

			default:
				g_set_error_literal(
					error,
					REPLIT_CLIENT_ERROR,
					REPLIT_CLIENT_ERROR_GRAPHQL_ERROR,
					"Server returned error in JSON response"
				);
				
				break;
		}
		
		json_node_unref(root);

		return NULL;
	}

	JsonNode* data_node = json_object_get_member(root_object, "data");

	if (data_node == NULL) {
		g_set_error_literal(
			error,
			REPLIT_CLIENT_ERROR,
			REPLIT_CLIENT_ERROR_GRAPHQL_EMPTY,
			"Server returned no data in JSON response"
		);

		json_node_unref(root);

		return NULL;
	}

	data_node = json_object_dup_member(root_object, "data");

	json_node_unref(root);

	return data_node;
}

/**
 * replit_client_query_to_object:
 * @client: The client.
 * @query: (transfer none): The GraphQL query or mutation to perform.
 * @variables: (transfer full) (nullable): Any variables to pass with the query.
 * @gtype: The object type to convert the response data to.
 * 
 * Sends a GraphQL query or mutation to Replit to perform as the current user,
 * and converts the response data to a GObject of the given type.
 * 
 * Internally, this method calls [method@Client.query] with its arguments, and
 * then uses GObject to turn the #JsonNode into the object.
 * 
 * Returns: (transfer full) (nullable): The object, or %NULL on error.
 */
GObject* replit_client_query_to_object(
	ReplitClient* self,
	const gchar* query,
	JsonNode* variables,
	GType gtype,
	GError** error
) {
	JsonNode* data = replit_client_query(self, query, variables, error);
	if (data == NULL) return NULL;
	GObject* object = json_gobject_deserialize(gtype, data);

	json_node_unref(data);

	return object;
}

/**
 * replit_client_login:
 * @username: (transfer none): The username to login with.
 * @password: (transfer none): The password to login with.
 * @captcha: (transfer none): The hCaptcha reponse key to send with the request.
 * 
 * Performs a login request with the given credentials to obtain a token.
 * 
 * Only password-based authentication is supported; you cannot obtain a token
 * with OAuth-based authentication using this library.
 * 
 * To obtain a code for @captcha, you must run the hCaptcha client library (in
 * JavaScript), which may require a human user to interactively solve a CAPTCHA
 * challenge.
 * 
 * Returns: (transfer full) (nullable): The obtained token, or %NULL on error.
 */
gchar* replit_client_login(
	const gchar* username,
	const gchar* password,
	const gchar* captcha,
	GError** error
) {
	JsonBuilder* builder = json_builder_new();
	json_builder_begin_object(builder);
	json_builder_set_member_name(builder, "username");
	json_builder_add_string_value(builder, username);
	json_builder_set_member_name(builder, "password");
	json_builder_add_string_value(builder, password);
	json_builder_set_member_name(builder, "teacher");
	json_builder_add_boolean_value(builder, FALSE);
	json_builder_set_member_name(builder, "hCaptchaResponse");
	json_builder_add_string_value(builder, captcha);
	json_builder_set_member_name(builder, "hCaptchaSiteKey");
	json_builder_add_string_value(builder, REPLIT_HC_KEY);
	json_builder_end_object(builder);

	JsonGenerator* generator = json_generator_new();
	JsonNode* builder_root = json_builder_get_root(builder);
	json_generator_set_root(generator, builder_root);

	gsize req_length;
	gchar* req_body = json_generator_to_data(generator, &req_length);

	g_object_unref(builder);
	g_object_unref(generator);
	g_object_unref(builder_root);

	GBytes* req_bytes = g_bytes_new(req_body, req_length);

	g_free(req_body);

	GUri* uri = g_uri_build(0, "https", NULL, REPLIT_DOMAIN, -1, "/login", NULL, NULL);
	SoupMessage* msg = soup_message_new_from_uri(SOUP_METHOD_POST, uri);
	soup_message_set_request_body_from_bytes(msg, "application/json", req_bytes);
	
	SoupMessageHeaders* headers = soup_message_get_request_headers(msg);
	soup_message_headers_append(headers, "Referrer", "https://replit.com/");
	soup_message_headers_append(headers, "User-Agent", "Mozilla/5.0");
	soup_message_headers_append(headers, "X-Requested-With", "XMLHttpRequest");
	soup_message_headers_append(headers, "X-Libreplit-Version", REPLIT_VERSION_S);

	SoupSession* session = soup_session_new();

	GInputStream* stream = soup_session_send(session, msg, NULL, error);

	SoupStatus status = soup_message_get_status(msg);

	g_object_unref(uri);
	g_object_unref(session);

	if (stream == NULL) {
		g_object_unref(msg);

		return NULL;
	}

	g_object_unref(stream);

	if (status != SOUP_STATUS_OK) {
		g_set_error(
			error,
			REPLIT_CLIENT_ERROR,
			REPLIT_CLIENT_ERROR_RESPONSE_STATUS,
			"Server responded with status %d",
			status
		);

		g_object_unref(msg);

		return NULL;
	}

	GSList* cookies = soup_cookies_from_response(msg);
	gchar* token = NULL;

	for (GSList* this = cookies; this; this = this->next) {
		SoupCookie* cookie = this->data;
		const char* cookie_name = soup_cookie_get_name(cookie);

		if (!g_str_equal(cookie_name, TOKEN_COOKIE)) continue;

		const char* cookie_value = soup_cookie_get_value(cookie);
		token = g_strdup(cookie_value);
		
		break;
	}

	g_object_unref(msg);
	g_slist_free(cookies);

	if (token == NULL) {
		g_set_error_literal(
			error,
			REPLIT_CLIENT_ERROR,
			REPLIT_CLIENT_ERROR_LOGIN_FAILED,
			"Server provided no token cookie"
		);

		return NULL;
	}

	return token;
}

/**
 * replit_client_get_subscriber:
 * @client: The client.
 * 
 * Returns an authenticated #ReplitSubscriber associated with the #ReplitClient.
 * 
 * The #ReplitClient holds the subscriber, and the #ReplitSubscriber is created
 * if it has not been already when this method is called. The #SoupSession
 * associated with the #ReplitClient is passed to the #ReplitSubscriber for
 * authentication purposes.
 * 
 * Returns: (transfer none): The subscriber instance.
 */
ReplitSubscriber* replit_client_get_subscriber(ReplitClient* self) {
	if (self->subscriber == NULL) {
		g_object_ref(self->session);
		self->subscriber = replit_subscriber_new_with_session(self->session);
	}

	return self->subscriber;
}

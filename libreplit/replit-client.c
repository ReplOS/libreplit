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

#include <glib.h>
#include <libsoup/soup.h>

#include "replit-client.h"

#define TOKEN_COOKIE "connect.sid"

G_DEFINE_QUARK (REPLIT_CLIENT_ERROR, replit_client_error)

struct _ReplitClient {
	GObject parent_instance;

	const gchar* token;
	const gchar* domain;
	SoupSession* session;
};

G_DEFINE_TYPE (ReplitClient, replit_client, G_TYPE_OBJECT)

static void replit_client_class_init(
	ReplitClientClass* _klass __attribute__((unused))
) {}

static void replit_client_init(ReplitClient* self) {
	self->domain = REPLIT_CLIENT_DEFAULT_DOMAIN;
}

ReplitClient* replit_client_new(const gchar* token) {
	return replit_client_new_with_domain(
		token,
		REPLIT_CLIENT_DEFAULT_DOMAIN
	);
}

ReplitClient* replit_client_new_with_domain(
	const gchar* token,
	const gchar* domain
) {
	SoupSession* session = soup_session_new();

	SoupCookie* cookie = soup_cookie_new(TOKEN_COOKIE, token, domain, "/", -1);

	SoupCookieJar* jar = soup_cookie_jar_new();
	soup_cookie_jar_add_cookie(jar, cookie);

	soup_session_add_feature(session, SOUP_SESSION_FEATURE (jar));

	return g_object_new(
		REPLIT_TYPE_CLIENT,
		"token", token,
		"domain", domain,
		"session", session,
		NULL
	);
}

JsonNode* replit_client_query(
	ReplitClient* self,
	const gchar* query,
	JsonNode* variables,
	GError** error
) {
	if (variables == NULL) variables = json_node_new(JSON_NODE_OBJECT);

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

	g_object_unref(builder);
	g_object_unref(generator);
	g_object_unref(builder_root);

	GBytes* req_bytes = g_bytes_new(req_body, req_length);

	g_free(req_body);

	GUri* uri = g_uri_build(0, "https", NULL, self->domain, -1, "/graphql", NULL, NULL);
	SoupMessage* msg = soup_message_new_from_uri(SOUP_METHOD_POST, uri);
	soup_message_set_request_body_from_bytes(msg, "application/json", req_bytes);

	GInputStream* stream = soup_session_send(self->session, msg, NULL, error);

	SoupStatus status = soup_message_get_status(msg);

	g_object_unref(uri);
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
		
		g_object_unref(root);

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

		g_object_unref(root);

		return NULL;
	}

	data_node = json_object_dup_member(root_object, "data");

	g_object_unref(root);

	return data_node;
}

GObject* replit_client_query_to_object(
	ReplitClient* self,
	const gchar* query,
	JsonNode* variables,
	GType gtype,
	GError** error
) {
	JsonNode* data = replit_client_query(self, query, variables, error);
	GObject* object = json_gobject_deserialize(gtype, data);

	g_object_unref(data);

	return object;
}
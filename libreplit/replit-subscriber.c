/* replit-subscriber.c
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

#include "replit-client.h"
#include "replit-subscriber.h"

#define TOKEN_COOKIE "connect.sid"

#define MESSAGE_INIT  "{\"type\":\"connection_init\",\"payload\":{}}"
#define MESSAGE_SUB   "{\"type\":\"start\",\"id\":%d,\"payload\":%s}"
#define MESSAGE_UNSUB "{\"type\":\"stop\",\"id\":%d}"

struct _ReplitSubscriber {
	GObject parent_instance;

	gchar* token;
	SoupSession* session;
	SoupCookieJar* jar;
	guint id_counter;
	GPtrArray* callbacks;
	GPtrArray* subscriptions;
	GPtrArray* user_data;
	SoupWebsocketConnection* ws;
};

G_DEFINE_TYPE (ReplitSubscriber, replit_subscriber, G_TYPE_OBJECT)

static void replit_subscriber_dispose(GObject* gobject);
static void replit_subscriber_finalize(GObject* gobject);
static void replit_subscriber_connect(ReplitSubscriber* subscriber);
static void replit_subscriber_connect_finish(
	GObject* source_object,
	GAsyncResult* res,
	gpointer user_data
);
static void replit_subscriber_on_message(
  SoupWebsocketConnection* ws,
  gint type,
  GBytes* message,
  gpointer user_data
);
static void replit_subscriber_on_close(
	SoupWebsocketConnection* ws,
	gpointer user_data
);

static void replit_subscriber_class_init(ReplitSubscriberClass* klass) {
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = replit_subscriber_dispose;
	object_class->finalize = replit_subscriber_finalize;
}

static void replit_subscriber_init(ReplitSubscriber* self) {
	self->callbacks = g_ptr_array_new();
	self->subscriptions = g_ptr_array_new_with_free_func(g_free);
	self->user_data = g_ptr_array_new();

	replit_subscriber_connect(self);
}

static void replit_subscriber_dispose(GObject* gobject) {
	ReplitSubscriber* self = REPLIT_SUBSCRIBER (gobject);

	g_clear_object(&self->session);
	g_clear_object(&self->jar);

	G_OBJECT_CLASS (replit_subscriber_parent_class)->dispose(gobject);
}

static void replit_subscriber_finalize(GObject* gobject) {
	ReplitSubscriber* self = REPLIT_SUBSCRIBER (gobject);

	g_ptr_array_remove_range(self->subscriptions, 0, self->subscriptions->len);

	g_free(self->token);
	g_ptr_array_free(self->callbacks, TRUE);
	g_ptr_array_free(self->subscriptions, TRUE);
	g_ptr_array_free(self->user_data, TRUE);

	G_OBJECT_CLASS (replit_subscriber_parent_class)->finalize(gobject);
}

static void replit_subscriber_connect(ReplitSubscriber* self) {
	GUri* uri = g_uri_build(
		0,
		"https",
		NULL,
		REPLIT_DOMAIN,
		-1,
		"/graphql_subscriptions",
		NULL,
		NULL
	);
	SoupMessage* msg = soup_message_new_from_uri(SOUP_METHOD_GET, uri);

	soup_session_websocket_connect_async(
		self->session,
		msg,
		NULL,
		NULL,
		G_PRIORITY_DEFAULT,
		NULL,
		replit_subscriber_connect_finish,
		self
	);
}

static void replit_subscriber_connect_finish(
	GObject* source_object __attribute__((unused)),
	GAsyncResult* res,
	gpointer user_data
) {
	ReplitSubscriber* self = REPLIT_SUBSCRIBER (user_data);

	self->ws = soup_session_websocket_connect_finish(self->session, res, NULL);

	if (self->ws == NULL) {
		replit_subscriber_connect(self);
		return;
	}

	g_signal_connect(self->ws, "message", (GCallback) replit_subscriber_on_message, self);
	g_signal_connect(self->ws, "closed", (GCallback) replit_subscriber_on_close, self);

	soup_websocket_connection_send_text(self->ws, MESSAGE_INIT);

	for (guint i = 0; i < self->subscriptions->len; i++) {
		if (g_ptr_array_index(self->subscriptions, i) == NULL) continue;

		gchar* message = g_ptr_array_index(self->subscriptions, i);
		soup_websocket_connection_send_text(self->ws, message);
	}
}

static void replit_subscriber_on_message(
  SoupWebsocketConnection* ws __attribute__((unused)),
  gint type,
  GBytes* message,
  gpointer user_data
) {
	ReplitSubscriber* self = REPLIT_SUBSCRIBER (user_data);

	if (type != SOUP_WEBSOCKET_DATA_TEXT) return;

	const gchar* data = g_bytes_get_data(message, NULL);
	JsonParser* parser = json_parser_new();
	
	if (!json_parser_load_from_data(parser, data, -1, NULL)) return;

	JsonNode* root = json_parser_get_root(parser);
	JsonObject* root_object = json_node_get_object(root);

	const gchar* msg_type = json_object_get_string_member(root_object, "type");

	if (!g_str_equal(msg_type, "data")) return;

	guint id = (guint) json_object_get_int_member(root_object, "id");
	JsonObject* payload = json_object_get_object_member(root_object, "payload");

	if (payload == NULL || id >= self->callbacks->len) return;

	JsonNode* node = json_node_copy(json_object_get_member(payload, "data"));
	gpointer callback_ptr = g_ptr_array_index(self->callbacks, id);
	
	g_object_unref(parser);

	if (callback_ptr == NULL) {
		g_object_unref(node);

		return;
	}

	ReplitSubscriptionCallback callback = (ReplitSubscriptionCallback) callback_ptr;
	callback(self, id, node, g_ptr_array_index(self->user_data, id));
}

static void replit_subscriber_on_close(
	SoupWebsocketConnection* ws __attribute__((unused)),
	gpointer user_data
) {
	ReplitSubscriber* self = REPLIT_SUBSCRIBER (user_data);

	g_clear_object(&self->ws);

	replit_subscriber_connect(self);
}

ReplitSubscriber* replit_subscriber_new(const gchar* token) {
	SoupSession* session = soup_session_new();

	SoupCookie* cookie = soup_cookie_new(TOKEN_COOKIE, token, REPLIT_DOMAIN, "/", -1);

	SoupCookieJar* jar = soup_cookie_jar_new();
	soup_cookie_jar_add_cookie(jar, cookie);

	soup_session_add_feature(session, SOUP_SESSION_FEATURE (jar));

	return g_object_new(
		REPLIT_TYPE_SUBSCRIBER,
		"token", g_strdup(token),
		"session", session,
		"jar", jar,
		NULL
	);
}

ReplitSubscriber* replit_subscriber_new_with_session(SoupSession* session) {
	return g_object_new(
		REPLIT_TYPE_SUBSCRIBER,
		"session", session,
		NULL
	);
}

guint replit_subscriber_subscribe(
	ReplitSubscriber* self,
	const gchar* query,
	JsonNode* variables,
	ReplitSubscriptionCallback callback,
	gpointer user_data
) {
	guint id = self->id_counter++;

	if (variables == NULL) variables = json_node_new(JSON_NODE_OBJECT);

	JsonNode* extensions = json_node_new(JSON_NODE_OBJECT);

	JsonBuilder* builder = json_builder_new();
	json_builder_begin_object(builder);
	json_builder_set_member_name(builder, "operationName");
	json_builder_add_null_value(builder);
	json_builder_set_member_name(builder, "query");
	json_builder_add_string_value(builder, query);
	json_builder_set_member_name(builder, "variables");
	json_builder_add_value(builder, variables);
	json_builder_set_member_name(builder, "extensions");
	json_builder_add_value(builder, extensions);
	json_builder_end_object(builder);

	JsonGenerator* generator = json_generator_new();
	JsonNode* builder_root = json_builder_get_root(builder);
	json_generator_set_root(generator, builder_root);

	gsize payload_length;
	gchar* payload = json_generator_to_data(generator, &payload_length);

	g_object_unref(extensions);
	g_object_unref(builder);
	g_object_unref(generator);
	g_object_unref(builder_root);

	gchar* message = g_strdup_printf(MESSAGE_SUB, id, payload);

	g_free(payload);

	if (self->ws != NULL) soup_websocket_connection_send_text(self->ws, message);

	g_ptr_array_add(self->callbacks, callback);
	g_ptr_array_add(self->subscriptions, message);
	g_ptr_array_add(self->user_data, user_data);

	return id;
}

typedef struct {
	ReplitSubscriptionCallbackObject callback;
	GType gtype;
	gpointer user_data;
} ReplitSubscriberObjectUserData;

static void replit_subscriber_object_callback(
	ReplitSubscriber* subscriber,
	guint id,
	JsonNode* data,
	gpointer user_data
) {
	ReplitSubscriberObjectUserData* user_data2 = user_data;

	GType gtype = user_data2->gtype;
	GObject* object = json_gobject_deserialize(gtype, data);

	user_data2->callback(subscriber, id, object, user_data2->user_data);
}

guint replit_subscriber_subscribe_to_object(
	ReplitSubscriber* self,
	const gchar* query,
	JsonNode* variables,
	GType gtype,
	ReplitSubscriptionCallbackObject callback,
	gpointer user_data
) {
	gsize size = sizeof(ReplitSubscriberObjectUserData);
	ReplitSubscriberObjectUserData* user_data2 = g_malloc(size);

	*user_data2 = (ReplitSubscriberObjectUserData) {
		.callback = callback,
		.gtype = gtype,
		.user_data = user_data
	};

	ReplitSubscriptionCallback callback2 = replit_subscriber_object_callback;
	return replit_subscriber_subscribe(self, query, variables, callback2, user_data2);
}

void replit_subscriber_unsubscribe(ReplitSubscriber* self, guint id) {
	if (id >= self->id_counter) return;

	if (self->subscriptions->pdata[id] != NULL) {
		g_free(self->subscriptions->pdata[id]);

		self->callbacks->pdata[id] = NULL;
		self->subscriptions->pdata[id] = NULL;
	}

	if (self->ws != NULL) {
		gchar* message = g_strdup_printf(MESSAGE_UNSUB, id);

		soup_websocket_connection_send_text(self->ws, message);

		g_free(message);
	}
}

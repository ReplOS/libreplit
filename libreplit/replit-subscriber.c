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

#include "replit-subscriber.h"

#define TOKEN_COOKIE "connect.sid"

struct _ReplitSubscriber {
	GObject parent_instance;

	gchar* token;
	SoupSession* session;
	SoupCookieJar* jar;
	guint id_counter;
	GPtrArray* callbacks;
	GPtrArray* subscriptions;
};

G_DEFINE_TYPE (ReplitSubscriber, replit_subscriber, G_TYPE_OBJECT)

static void replit_subscriber_dispose(GObject* gobject);
static void replit_subscriber_finalize(GObject* gobject);

static void replit_subscriber_class_init(ReplitSubscriberClass* klass) {
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = replit_subscriber_dispose;
	object_class->finalize = replit_subscriber_finalize;
}

static void replit_subscriber_init(ReplitSubscriber* self __attribute__((unused))) {}

static void replit_subscriber_dispose(GObject* gobject) {
	ReplitSubscriber* self = REPLIT_SUBSCRIBER (gobject);

	g_clear_object(&self->session);
	g_clear_object(&self->jar);

	G_OBJECT_CLASS (replit_subscriber_parent_class)->dispose(gobject);
}

static void replit_subscriber_finalize(GObject* gobject) {
	ReplitSubscriber* self = REPLIT_SUBSCRIBER (gobject);

	g_free(self->token);
	g_free(self->callbacks);
	g_free(self->subscriptions);

	G_OBJECT_CLASS (replit_subscriber_parent_class)->finalize(gobject);
}

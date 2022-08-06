/* replit-subscriber.h
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
#include <libsoup/soup.h>

G_BEGIN_DECLS

typedef void (* ReplitSubscriptionCallback)(
	ReplitSubscriber* subscriber,
	guint id,
	JsonNode* data
);

typedef void (* ReplitSubscriptionCallbackObject)(
	ReplitSubscriber* subscriber,
	guint id,
	GObject* object
);

#define REPLIT_TYPE_SUBSCRIBER replit_subscriber_get_type()
G_DECLARE_FINAL_TYPE (ReplitSubscriber, replit_subscriber, REPLIT, SUBSCRIBER, GObject)

ReplitSubscriber* replit_subscriber_new(SoupSession* session);

guint replit_subscriber_subscribe(
	ReplitSubscriber* subscriber,
	const gchar* query,
	JsonNode* variables,
	ReplitSubscriptionCallback callback
);

guint replit_subscriber_subscribe_to_object(
	ReplitSubscriber* subscriber,
	const gchar* query,
	JsonNode* variables,
	GType gtype,
	ReplitSubscriptionCallbackObject callback
);

void replit_subscriber_unsubscribe(
	ReplitSubscriber* subscriber,
	guint id
);

G_END_DECLS

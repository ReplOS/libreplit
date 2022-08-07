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

#define REPLIT_TYPE_SUBSCRIBER replit_subscriber_get_type()
G_DECLARE_FINAL_TYPE (ReplitSubscriber, replit_subscriber, REPLIT, SUBSCRIBER, GObject)

/**
 * ReplitSubscriptionCallback:
 * @subscriber: The subscriber.
 * @id: The ID of the subscription.
 * @data: (transfer full): The data received from Replit.
 * @user_data: (transfer none) (nullable): Any user data given when subscribing.
 * 
 * A callback for when new data is received as part of a subscription.
 */
typedef void (* ReplitSubscriptionCallback)(
	ReplitSubscriber* subscriber,
	guint id,
	JsonNode* data,
	gpointer user_data
);

/**
 * ReplitSubscriptionCallbackObject:
 * @subscriber: The subscriber.
 * @id: The ID of the subscription.
 * @object: (transfer full): The data received, converted to a #GObject.
 * @user_data: (transfer none) (nullable): Any user data given when subscribing.
 * 
 * A callback for when new data is received as part of a subscription.
 * 
 * This callback form is used for when the response data is given as part of a
 * subscription where a #GType to convert to has been passed.
 */
typedef void (* ReplitSubscriptionCallbackObject)(
	ReplitSubscriber* subscriber,
	guint id,
	GObject* object,
	gpointer user_data
);

ReplitSubscriber* replit_subscriber_new(const gchar* token);

ReplitSubscriber* replit_subscriber_new_with_session(SoupSession* session);

guint replit_subscriber_subscribe(
	ReplitSubscriber* subscriber,
	const gchar* query,
	JsonNode* variables,
	ReplitSubscriptionCallback callback,
	gpointer user_data
);

guint replit_subscriber_subscribe_to_object(
	ReplitSubscriber* subscriber,
	const gchar* query,
	JsonNode* variables,
	GType gtype,
	ReplitSubscriptionCallbackObject callback,
	gpointer user_data
);

void replit_subscriber_unsubscribe(ReplitSubscriber* subscriber, guint id);

G_END_DECLS

// Copyright (c) 2026 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 3.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Session pseudo device
 \file indigo_session.c
 */

#include <indigo/indigo_bus.h>
#include <indigo/indigo_names.h>
#include <indigo/indigo_session.h>

static indigo_property *session_features_property;

#define SESSION_DEVICE_NAME										"Server"

#define SESSION_FEATURES_PROPERTY							session_features_property
#define SESSION_FORCE_PROPERTY_UPDATES_ITEM		(SESSION_FEATURES_PROPERTY->items + 0)
#define SESSION_FORCE_ITEM_UPDATES_ITEM				(SESSION_FEATURES_PROPERTY->items + 1)

static indigo_result session_attach(indigo_device *device);
static indigo_result session_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
static indigo_result session_change_property(indigo_device *device, indigo_client *client, indigo_property *property);

indigo_device indigo_session_device = INDIGO_DEVICE_INITIALIZER(SESSION_DEVICE_NAME, session_attach, session_enumerate_properties, session_change_property, NULL, NULL);

static indigo_result session_attach(indigo_device *device) {
	session_features_property = indigo_init_switch_property(SESSION_FEATURES_PROPERTY, SESSION_DEVICE_NAME, SESSION_FEATURES_PROPERTY_NAME, "Session", "Session features", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
	indigo_init_switch_item(SESSION_FORCE_PROPERTY_UPDATES_ITEM, SESSION_FORCE_PROPERTY_UPDATES_ITEM_NAME, "Force redundant property updates", false);
	indigo_init_switch_item(SESSION_FORCE_ITEM_UPDATES_ITEM, SESSION_FORCE_ITEM_UPDATES_ITEM_NAME, "Force redundant item updates", false);
	return INDIGO_OK;
}

static indigo_result session_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client != NULL && indigo_property_match(SESSION_FEATURES_PROPERTY, property)) {
		SESSION_FORCE_PROPERTY_UPDATES_ITEM->sw.value = client->force_property_updates;
		SESSION_FORCE_ITEM_UPDATES_ITEM->sw.value = client->force_item_updates;
		indigo_define_property_to_client(&indigo_session_device, client, SESSION_FEATURES_PROPERTY, NULL);
	}
	return INDIGO_OK;
}

static indigo_result session_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (client != NULL && indigo_property_match_changeable(SESSION_FEATURES_PROPERTY, property)) {
		indigo_property_copy_values(SESSION_FEATURES_PROPERTY, property, false);
		client->force_property_updates = SESSION_FORCE_PROPERTY_UPDATES_ITEM->sw.value;
		client->force_item_updates = SESSION_FORCE_ITEM_UPDATES_ITEM->sw.value;
		SESSION_FORCE_PROPERTY_UPDATES_ITEM->do_update = SESSION_FORCE_ITEM_UPDATES_ITEM->do_update = true;
		indigo_update_property_to_client(&indigo_session_device, client, SESSION_FEATURES_PROPERTY, NULL);
		indigo_debug("Session features updated for %s", client->name);
		return INDIGO_OK;
	}
	return INDIGO_NOT_FOUND;
}

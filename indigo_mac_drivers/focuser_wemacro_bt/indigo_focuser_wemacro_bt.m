// Copyright (c) 2018 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO WeMacro Rail bluetooth driver
 \file indigo_focuser_wemacro_bt.m
 */

#define DRIVER_VERSION 0x0002
#define DRIVER_NAME "indigo_focuser_wemacro_bt"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

#import <CoreBluetooth/CoreBluetooth.h>

#include "indigo_focuser_wemacro_bt.h"

#define PRIVATE_DATA													((wemacro_private_data *)device->private_data)

#define X_RAIL_BATCH													"Batch"

#define X_RAIL_CONFIG_PROPERTY								(PRIVATE_DATA->config_property)
#define X_RAIL_CONFIG_BACK_ITEM								(X_RAIL_CONFIG_PROPERTY->items+0)
#define X_RAIL_CONFIG_BEEP_ITEM								(X_RAIL_CONFIG_PROPERTY->items+1)

#define X_RAIL_SHUTTER_PROPERTY								(PRIVATE_DATA->shutter_property)
#define X_RAIL_SHUTTER_ITEM										(X_RAIL_SHUTTER_PROPERTY->items+0)

#define X_RAIL_EXECUTE_PROPERTY								(PRIVATE_DATA->move_steps_property)
#define X_RAIL_EXECUTE_SETTLE_TIME_ITEM				(X_RAIL_EXECUTE_PROPERTY->items+0)
#define X_RAIL_EXECUTE_PER_STEP_ITEM					(X_RAIL_EXECUTE_PROPERTY->items+1)
#define X_RAIL_EXECUTE_INTERVAL_ITEM					(X_RAIL_EXECUTE_PROPERTY->items+2)
#define X_RAIL_EXECUTE_LENGTH_ITEM						(X_RAIL_EXECUTE_PROPERTY->items+3)
#define X_RAIL_EXECUTE_COUNT_ITEM							(X_RAIL_EXECUTE_PROPERTY->items+4)

typedef struct {
	int device_count;
	pthread_t reader;
	indigo_property *shutter_property;
	indigo_property *config_property;
	indigo_property *move_steps_property;
} wemacro_private_data;

static indigo_result focuser_attach(indigo_device *device);
static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
static indigo_result focuser_detach(indigo_device *device);

@interface WeMacroBTDelegate : NSObject<CBCentralManagerDelegate, CBPeripheralDelegate>
-(void)connect;
-(void)cmd:(uint8_t)cmd a:(uint8_t)a b:(uint8_t)b c:(uint8_t)c d:(uint32_t)d;
-(void)disconnect;
@end

#pragma clang diagnostic ignored "-Wshadow-ivar"

@implementation WeMacroBTDelegate {
	CBCentralManager *central;
	CBPeripheral *hc08;
	CBCharacteristic *ffe1;
	bool scanning;
	wemacro_private_data *private_data;
	indigo_device *device;
}

-(id)init {
	self = [super init];
	if (self) {
		central = [[CBCentralManager alloc] initWithDelegate:self queue:nil];
		hc08 = nil;
		ffe1 = nil;
		scanning = false;
		private_data = NULL;
		device = NULL;
	}
	return self;
}

-(void)createDevice {
	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		FOCUSER_WEMACRO_BT_NAME,
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
		);
	private_data = malloc(sizeof(wemacro_private_data));
	assert(private_data != NULL);
	memset(private_data, 0, sizeof(wemacro_private_data));
	device = malloc(sizeof(indigo_device));
	assert(device != NULL);
	memcpy(device, &focuser_template, sizeof(indigo_device));
	device->private_data = private_data;
	indigo_attach_device(device);
}

-(void)deleteDevice {
	if (device != NULL) {
		indigo_detach_device(device);
		free(device);
		device = NULL;
	}
	if (private_data != NULL) {
		free(private_data);
		private_data = NULL;
	}
}

-(void)centralManagerDidUpdateState:(CBCentralManager *)central {
	switch (central.state) {
		case CBManagerStatePoweredOff:
			@synchronized(self) {
				if (scanning) {
					scanning = false;
					INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Scanning stopped");
				}
				[self deleteDevice];
				hc08 = nil;
				ffe1 = nil;
				if (hc08)
					CFBridgingRelease((__bridge void *)hc08);
			}
			break;
		case CBManagerStatePoweredOn:
			if (hc08 == nil) {
				for (CBPeripheral *peripheral in [central retrieveConnectedPeripheralsWithServices:@[[CBUUID UUIDWithString:@"FFE0"]]]) {
					if ([peripheral.name isEqualToString:@"HC-08"]) {
						CFBridgingRetain(peripheral);
						hc08 = peripheral;
						peripheral.delegate = self;
						[self createDevice];
						break;
					}
				}
			}
			if (hc08 == nil) {
				@synchronized(self) {
					if (!scanning) {
						[central scanForPeripheralsWithServices:nil options:nil];
						scanning = true;
						INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Scanning started");
					}
				}
			}
			break;
		default:
			break;
	}
}

-(void)centralManager:(CBCentralManager *)central didDiscoverPeripheral:(CBPeripheral *)peripheral advertisementData:(NSDictionary<NSString *,id> *)advertisementData RSSI:(NSNumber *)RSSI {
	if ([peripheral.name isEqualToString:@"HC-08"]) {
		CFBridgingRetain(peripheral);
		hc08 = peripheral;
		peripheral.delegate = self;
		[self createDevice];
		if (scanning) {
			scanning = false;
			[central stopScan];
			INDIGO_DRIVER_DEBUG(DRIVER_NAME, "Scanning stopped");
		}
	}
}

-(void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral {
	[hc08 discoverServices:@[[CBUUID UUIDWithString:@"FFE0"]]];
}

-(void)centralManager:(CBCentralManager *)central didDisconnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error {
	indigo_delete_property(device, X_RAIL_CONFIG_PROPERTY, NULL);
	indigo_delete_property(device, X_RAIL_SHUTTER_PROPERTY, NULL);
	indigo_delete_property(device, X_RAIL_EXECUTE_PROPERTY, NULL);
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
	//indigo_update_property(device, CONNECTION_PROPERTY, NULL);
	indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
	INDIGO_DRIVER_LOG(DRIVER_NAME, "disconnected from %s", device->name);
}

-(void)peripheral:(CBPeripheral *)peripheral didDiscoverServices:(NSError *)error {
	for (CBService *service in peripheral.services) {
		if ([service.UUID isEqual:[CBUUID UUIDWithString:@"FFE0"]]) {
			[peripheral discoverCharacteristics:@[[CBUUID UUIDWithString:@"FFE1"]] forService:service];
		}
	}
}

-(void)peripheral:(CBPeripheral *)peripheral didDiscoverCharacteristicsForService:(CBService *)service error:(NSError *)error {
	for (CBCharacteristic *characteristic in service.characteristics) {
		if ([characteristic.UUID isEqual:[CBUUID UUIDWithString:@"FFE1"]]) {
			ffe1 = characteristic;
			[peripheral setNotifyValue:true forCharacteristic:characteristic];
			INDIGO_DRIVER_LOG(DRIVER_NAME, "connected to %s", device->name);
			indigo_define_property(device, X_RAIL_CONFIG_PROPERTY, NULL);
			indigo_define_property(device, X_RAIL_SHUTTER_PROPERTY, NULL);
			indigo_define_property(device, X_RAIL_EXECUTE_PROPERTY, NULL);
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_CONNECTED_ITEM, true);
			//indigo_update_property(device, CONNECTION_PROPERTY, NULL);
			indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
			[self cmd:0x40 a:0 b:0 c:0 d:0];
			[self cmd:0x20 a:0 b:0 c:0 d:0];
			[self cmd:0x40 a:0 b:0 c:0 d:0];
			usleep(100000);
			[self cmd:0x80 | (X_RAIL_CONFIG_BEEP_ITEM->sw.value ? 0x02 : 0) | (X_RAIL_CONFIG_BACK_ITEM->sw.value ? 0x08 : 0) a:FOCUSER_SPEED_ITEM->number.value == 2 ? 0xFF : 0 b:0 c:0 d:0];
		}
	}
}

-(void)peripheral:(CBPeripheral *)peripheral didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error {
	uint8_t *buffer = (uint8_t *)characteristic.value.bytes;
	uint8_t state = buffer[2];
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%02x %02x %02x", buffer[0], buffer[1], buffer[2]);
	if (state) {
		if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (state == 0xf5 || state == 0xf6) {
				FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			}
		} else if (X_RAIL_EXECUTE_PROPERTY->state == INDIGO_BUSY_STATE) {
			if (state == 0xf7) {
				X_RAIL_EXECUTE_COUNT_ITEM->number.value--;
			}
			if (X_RAIL_CONFIG_BACK_ITEM->sw.value) {
				if (state == 0xf6)
					X_RAIL_EXECUTE_PROPERTY->state = INDIGO_OK_STATE;
			} else if (X_RAIL_EXECUTE_COUNT_ITEM->number.value == 0) {
				X_RAIL_EXECUTE_PROPERTY->state = INDIGO_OK_STATE;
			}
			indigo_update_property(device, X_RAIL_EXECUTE_PROPERTY, NULL);
		}
	}
}

-(void)connect {
	if (hc08.state == CBPeripheralStateConnected)
		[hc08 discoverServices:@[[CBUUID UUIDWithString:@"FFE0"]]];
	else
		[central connectPeripheral:hc08 options:nil];
}

-(void)cmd:(uint8_t)cmd a:(uint8_t)a b:(uint8_t)b c:(uint8_t)c d:(uint32_t)d {
	uint8_t out[12] = { 0xA5, 0x5A, cmd, a, b, c, (d >> 24 & 0xFF), (d >> 16 & 0xFF), (d >> 8 & 0xFF), (d & 0xFF) };
	uint16_t crc = 0xFFFF;
	for (int i = 0; i < 10; i++) {
		crc = crc ^ out[i];
		for (int j = 0; j < 8; j++) {
			if (crc & 0x0001)
				crc = (crc >> 1) ^ 0xA001;
			else
				crc = crc >> 1;
		}
	}
	out[10] = crc & 0xFF;
	out[11] = (crc >> 8) &0xFF;
	[hc08 writeValue:[NSData dataWithBytes:out length:12] forCharacteristic:ffe1 type:CBCharacteristicWriteWithoutResponse];
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7], out[8], out[9], out[10], out[11]);
}

-(void)disconnect {
	if (hc08.state != CBPeripheralStateDisconnected)
		[central cancelPeripheralConnection:hc08];
}

@end

static WeMacroBTDelegate *delegate;

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = true;
		DEVICE_PORTS_PROPERTY->hidden = true;
		FOCUSER_REVERSE_MOTION_PROPERTY->hidden = false;
		FOCUSER_POSITION_PROPERTY->hidden = true;
		FOCUSER_SPEED_ITEM->number.value = FOCUSER_SPEED_ITEM->number.target = 1;
		FOCUSER_SPEED_ITEM->number.max = 2;
		// -------------------------------------------------------------------------------- X_RAIL_CONFIG
		X_RAIL_CONFIG_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_RAIL_CONFIG", X_RAIL_BATCH, "Set configuration", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 2);
		if (X_RAIL_CONFIG_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_RAIL_CONFIG_BACK_ITEM, "BACK", "Return back when done", false);
		indigo_init_switch_item(X_RAIL_CONFIG_BEEP_ITEM, "BEEP", "Beep when done", false);
		// -------------------------------------------------------------------------------- X_RAIL_SHUTTER
		X_RAIL_SHUTTER_PROPERTY = indigo_init_switch_property(NULL, device->name, "X_RAIL_SHUTTER", X_RAIL_BATCH, "Fire shutter", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_ANY_OF_MANY_RULE, 1);
		if (X_RAIL_SHUTTER_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_switch_item(X_RAIL_SHUTTER_ITEM, "SHUTTER", "Fire shutter", false);
		// -------------------------------------------------------------------------------- X_RAIL_EXECUTE
		X_RAIL_EXECUTE_PROPERTY = indigo_init_number_property(NULL, device->name, "X_RAIL_EXECUTE", X_RAIL_BATCH, "Execute batch", INDIGO_OK_STATE, INDIGO_RW_PERM, 5);
		if (X_RAIL_EXECUTE_PROPERTY == NULL)
			return INDIGO_FAILED;
		indigo_init_number_item(X_RAIL_EXECUTE_SETTLE_TIME_ITEM, "SETTLE_TIME", "Settle time", 0, 99, 1, 1);
		indigo_init_number_item(X_RAIL_EXECUTE_PER_STEP_ITEM, "SHUTTER_PER_STEP", "Shutter per step", 1, 9, 1, 1);
		indigo_init_number_item(X_RAIL_EXECUTE_INTERVAL_ITEM, "SHUTTER_INTERVAL", "Shutter interval", 1, 99, 1, 1);
		indigo_init_number_item(X_RAIL_EXECUTE_LENGTH_ITEM, "LENGTH", "Step size", 1, 0xFFFFFF, 1, 1);
		indigo_init_number_item(X_RAIL_EXECUTE_COUNT_ITEM, "COUNT", "Step count", 0, 0xFFFFFF, 1, 1);
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	if (IS_CONNECTED) {
		if (indigo_property_match(X_RAIL_CONFIG_PROPERTY, property))
			indigo_define_property(device, X_RAIL_CONFIG_PROPERTY, NULL);
		if (indigo_property_match(X_RAIL_SHUTTER_PROPERTY, property))
			indigo_define_property(device, X_RAIL_SHUTTER_PROPERTY, NULL);
		if (indigo_property_match(X_RAIL_EXECUTE_PROPERTY, property))
			indigo_define_property(device, X_RAIL_EXECUTE_PROPERTY, NULL);
	}
	return indigo_focuser_enumerate_properties(device, NULL, NULL);
}

static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	if (indigo_property_match(CONNECTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- CONNECTION
		indigo_property_copy_values(CONNECTION_PROPERTY, property, false);
		if (CONNECTION_CONNECTED_ITEM->sw.value) {
			if (PRIVATE_DATA->device_count++ == 0) {
				[delegate connect];
				CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			}
		} else {
			if (--PRIVATE_DATA->device_count == 0) {
				[delegate disconnect];
				CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
			}
		}
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		if (FOCUSER_STEPS_PROPERTY->state != INDIGO_BUSY_STATE) {
			FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
			if (FOCUSER_DIRECTION_MOVE_INWARD_ITEM->sw.value) {
				[delegate cmd:FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value ? 0x41 : 0x40 a:0 b:0 c:0 d:FOCUSER_STEPS_ITEM->number.value];
			} else {
				[delegate cmd:FOCUSER_REVERSE_MOTION_DISABLED_ITEM->sw.value ? 0x40 : 0x41 a:0 b:0 c:0 d:FOCUSER_STEPS_ITEM->number.value];
			}
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
	// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
			[delegate cmd:0x20 a:0 b:0 c:0 d:0];
			X_RAIL_EXECUTE_COUNT_ITEM->number.value = 0;
			X_RAIL_EXECUTE_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, X_RAIL_EXECUTE_PROPERTY, NULL);
			FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
			indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		}
		FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
		FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
		indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
    if (IS_CONNECTED)
			[delegate cmd:0x80 | (X_RAIL_CONFIG_BEEP_ITEM->sw.value ? 0x02 : 0) | (X_RAIL_CONFIG_BACK_ITEM->sw.value ? 0x08 : 0) a:FOCUSER_SPEED_ITEM->number.value == 2 ? 0xFF : 0 b:0 c:0 d:0];
		FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_RAIL_CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RAIL_CONFIG
		indigo_property_copy_values(X_RAIL_CONFIG_PROPERTY, property, false);
		X_RAIL_CONFIG_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_RAIL_CONFIG_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_RAIL_SHUTTER_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RAIL_SHUTTER
		indigo_property_copy_values(X_RAIL_SHUTTER_PROPERTY, property, false);
		if (X_RAIL_SHUTTER_ITEM->sw.value) {
			[delegate cmd:0x04 a:0 b:0 c:0 d:0];
			X_RAIL_SHUTTER_ITEM->sw.value = false;
		}
		X_RAIL_SHUTTER_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, X_RAIL_SHUTTER_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(X_RAIL_EXECUTE_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- X_RAIL_EXECUTE
		indigo_property_copy_values(X_RAIL_EXECUTE_PROPERTY, property, false);
		[delegate cmd:0x80 a:FOCUSER_SPEED_ITEM->number.value == 2 ? 0xFF : 0 b:0 c:0 d:(uint32_t)X_RAIL_EXECUTE_LENGTH_ITEM->number.value];
		usleep(100000);
		[delegate cmd:0x10 | (X_RAIL_CONFIG_BEEP_ITEM->sw.value ? 0x02 : 0) | (X_RAIL_CONFIG_BACK_ITEM->sw.value ? 0x08 : 0) a:(uint8_t)X_RAIL_EXECUTE_SETTLE_TIME_ITEM->number.value b:(uint8_t)X_RAIL_EXECUTE_PER_STEP_ITEM->number.value c:(uint8_t)X_RAIL_EXECUTE_INTERVAL_ITEM->number.value d:(uint32_t)X_RAIL_EXECUTE_COUNT_ITEM->number.value - 1];
		X_RAIL_EXECUTE_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, X_RAIL_EXECUTE_PROPERTY, NULL);
		return INDIGO_OK;
	} else if (indigo_property_match(CONFIG_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- CONFIG
		if (indigo_switch_match(CONFIG_SAVE_ITEM, property)) {
			indigo_save_property(device, NULL, X_RAIL_CONFIG_PROPERTY);
		}
	// --------------------------------------------------------------------------------
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	indigo_release_property(X_RAIL_CONFIG_PROPERTY);
	indigo_release_property(X_RAIL_SHUTTER_PROPERTY);
	indigo_release_property(X_RAIL_EXECUTE_PROPERTY);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// --------------------------------------------------------------------------------

indigo_result indigo_focuser_wemacro_bt(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	
	SET_DRIVER_INFO(info, FOCUSER_WEMACRO_BT_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			delegate = [[WeMacroBTDelegate alloc] init];
			break;

		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			delegate = nil;
			break;

	case INDIGO_DRIVER_INFO:
		break;
	}

	return INDIGO_OK;
}

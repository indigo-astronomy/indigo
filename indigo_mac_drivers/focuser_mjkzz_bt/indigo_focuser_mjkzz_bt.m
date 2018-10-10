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

/** INDIGO MJKZZ Rail bluetooth driver
 \file indigo_focuser_mjkzz_bt.m
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_ccd_mjkzz_bt"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

#import <CoreBluetooth/CoreBluetooth.h>

#include "indigo_driver_xml.h"
#include "indigo_focuser_mjkzz_bt.h"

#include "focuser_mjkzz/mjkzz_def.h"

#define PRIVATE_DATA													((mjkzz_private_data *)device->private_data)
typedef struct {
	int handle;
} mjkzz_private_data;

static indigo_result focuser_attach(indigo_device *device);
static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);
static indigo_result focuser_change_property(indigo_device *device, indigo_client *client, indigo_property *property);
static indigo_result focuser_detach(indigo_device *device);

@interface MJKZZBTDelegate : NSObject<CBCentralManagerDelegate, CBPeripheralDelegate>
-(void)connect;
-(void)command:(char)cmd index:(int)index value:(int)value;
-(void)disconnect;
@end

#pragma clang diagnostic ignored "-Wshadow-ivar"

@implementation MJKZZBTDelegate {
	CBCentralManager *central;
	CBPeripheral *stackrail;
	CBCharacteristic *ffe1;
	bool scanning;
	mjkzz_private_data *private_data;
	indigo_device *device;
	NSLock *lock;
}

-(id)init {
	self = [super init];
	if (self) {
		central = [[CBCentralManager alloc] initWithDelegate:self queue:nil];
		stackrail = nil;
		ffe1 = nil;
		scanning = false;
		private_data = NULL;
		device = NULL;
		lock = [[NSLock alloc] init];
	}
	return self;
}

-(void)createDevice {
	static indigo_device focuser_template = INDIGO_DEVICE_INITIALIZER(
		FOCUSER_MJKZZ_BT_NAME,
		focuser_attach,
		focuser_enumerate_properties,
		focuser_change_property,
		NULL,
		focuser_detach
		);
	private_data = malloc(sizeof(mjkzz_private_data));
	assert(private_data != NULL);
	memset(private_data, 0, sizeof(mjkzz_private_data));
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
				stackrail = nil;
				ffe1 = nil;
			}
			break;
		case CBManagerStatePoweredOn:
			if (stackrail == nil) {
				for (CBPeripheral *peripheral in [central retrieveConnectedPeripheralsWithServices:@[[CBUUID UUIDWithString:@"FFE0"]]]) {
					if ([peripheral.name isEqualToString:@"STACKRAIL"]) {
						stackrail = peripheral;
						peripheral.delegate = self;
						[self createDevice];
						break;
					}
				}
			}
			if (stackrail == nil) {
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
	if ([peripheral.name isEqualToString:@"STACKRAIL"]) {
		stackrail = peripheral;
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
	[stackrail discoverServices:@[[CBUUID UUIDWithString:@"FFE0"]]];
}

-(void)centralManager:(CBCentralManager *)central didDisconnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error {
	CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
	indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_DISCONNECTED_ITEM, true);
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
			CONNECTION_PROPERTY->state = INDIGO_OK_STATE;
			indigo_set_switch(CONNECTION_PROPERTY, CONNECTION_CONNECTED_ITEM, true);
			indigo_focuser_change_property(device, NULL, CONNECTION_PROPERTY);
			[self command:CMD_SREG index:reg_HPWR value:16];
			[self command:CMD_SREG index:reg_LPWR value:2];
			[self command:CMD_SREG index:reg_MSTEP value:0];
			[self command:CMD_GPOS index:0 value:0];
			[self command:CMD_GSPD index:0 value:0];
		}
	}
}

-(void)peripheral:(CBPeripheral *)peripheral didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error {
	mjkzz_message *message = (mjkzz_message *)characteristic.value.bytes;
	INDIGO_DRIVER_DEBUG(DRIVER_NAME, "< %02x%02x%02x%02x%02x%02x%02x%02x", message->ucADD, message->ucCMD, message->ucIDX, message->ucMSG[0], message->ucMSG[1], message->ucMSG[2], message->ucMSG[3], message->ucSUM);
	if (message->ucADD == 0x81) {
		int value = ((((((int32_t)message->ucMSG[0] << 8) + (int32_t)message->ucMSG[1]) << 8) + (int32_t)message->ucMSG[2]) << 8) + (int32_t)message->ucMSG[3];
		switch (message->ucCMD & 0x7F) {
			case CMD_GPOS: {
				FOCUSER_POSITION_ITEM->number.value =value;
				if (FOCUSER_POSITION_PROPERTY->state == INDIGO_OK_STATE) {
					FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value;
				} else if (FOCUSER_POSITION_PROPERTY->state == INDIGO_BUSY_STATE && FOCUSER_POSITION_ITEM->number.value == FOCUSER_POSITION_ITEM->number.target) {
					FOCUSER_POSITION_PROPERTY->state = INDIGO_OK_STATE;
					if (FOCUSER_STEPS_PROPERTY->state == INDIGO_BUSY_STATE) {
						FOCUSER_STEPS_PROPERTY->state = INDIGO_OK_STATE;
						indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
					}
				} else {
					dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0.3 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
						[self command:CMD_GPOS index:0 value:0];
					});
				}
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
				break;
			}
			case CMD_GSPD: {
				int speed = value;
				if (speed > FOCUSER_SPEED_ITEM->number.max)
					speed = (int)FOCUSER_SPEED_ITEM->number.max;
				FOCUSER_SPEED_ITEM->number.value = speed;
				if (FOCUSER_SPEED_PROPERTY->state == INDIGO_OK_STATE) {
					FOCUSER_SPEED_ITEM->number.target = FOCUSER_SPEED_ITEM->number.value;
				}
				FOCUSER_SPEED_PROPERTY->state = INDIGO_OK_STATE;
				indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
				break;
			}
			case CMD_STOP: {
				FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value = value;
				FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_OK_STATE;
				FOCUSER_STEPS_PROPERTY->state = INDIGO_ALERT_STATE;
				FOCUSER_POSITION_PROPERTY->state = INDIGO_ALERT_STATE;
				indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
				indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
				indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
				break;
			}
		}
	}
}

-(void)connect {
	if (stackrail.state == CBPeripheralStateConnected)
		[stackrail discoverServices:@[[CBUUID UUIDWithString:@"FFE0"]]];
	else
		[central connectPeripheral:stackrail options:nil];
}

-(void)command:(char)cmd index:(int)index value:(int)value {
	mjkzz_message message;
	message.ucADD = 0x01;
	message.ucCMD = cmd;
	message.ucIDX = index;
	message.ucMSG[0] = (uint8_t)(value >> 24);
	message.ucMSG[1] = (uint8_t)(value >> 16);
	message.ucMSG[2] = (uint8_t)(value >> 8);
	message.ucMSG[3] = (uint8_t)value;
	message.ucSUM = message.ucADD + message.ucCMD + message.ucIDX + message.ucMSG[0] + message.ucMSG[1] + message.ucMSG[2] + message.ucMSG[3];
	dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		[lock lock];
		INDIGO_DRIVER_DEBUG(DRIVER_NAME, "> %02x%02x%02x%02x%02x%02x%02x%02x", message.ucADD, message.ucCMD, message.ucIDX, message.ucMSG[0], message.ucMSG[1], message.ucMSG[2], message.ucMSG[3], message.ucSUM);
		[stackrail writeValue:[NSData dataWithBytes:&message length:8] forCharacteristic:ffe1 type:CBCharacteristicWriteWithoutResponse];
		usleep(200000);
		[lock unlock];
	});
}

-(void)disconnect {
	if (stackrail.state != CBPeripheralStateDisconnected)
		[central cancelPeripheralConnection:stackrail];
}

@end

static MJKZZBTDelegate *delegate;

// -------------------------------------------------------------------------------- INDIGO focuser device implementation

static indigo_result focuser_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_focuser_attach(device, DRIVER_VERSION) == INDIGO_OK) {
		DEVICE_PORT_PROPERTY->hidden = true;
		DEVICE_PORTS_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_ROTATION
		FOCUSER_ROTATION_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_TEMPERATURE
		FOCUSER_TEMPERATURE_PROPERTY->hidden = true;
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		FOCUSER_SPEED_ITEM->number.min = 0;
		FOCUSER_SPEED_ITEM->number.max = 3;
		FOCUSER_SPEED_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		FOCUSER_STEPS_ITEM->number.min = 0;
		FOCUSER_STEPS_ITEM->number.max = 1000;
		FOCUSER_STEPS_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		FOCUSER_POSITION_PROPERTY->hidden = false;
		FOCUSER_POSITION_ITEM->number.min = -32768;
		FOCUSER_POSITION_ITEM->number.max = 32767;
		FOCUSER_POSITION_ITEM->number.step = 1;
		// -------------------------------------------------------------------------------- FOCUSER_ROTATION
		FOCUSER_ROTATION_PROPERTY->hidden = true;
		// --------------------------------------------------------------------------------
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_focuser_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

static indigo_result focuser_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
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
			[delegate connect];
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		} else {
			[delegate disconnect];
			CONNECTION_PROPERTY->state = INDIGO_BUSY_STATE;
		}
	} else if (indigo_property_match(FOCUSER_SPEED_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_SPEED
		if (IS_CONNECTED) {
			indigo_property_copy_values(FOCUSER_SPEED_PROPERTY, property, false);
			FOCUSER_SPEED_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_SPEED_PROPERTY, NULL);
			[delegate command:CMD_SSPD index:0 value:FOCUSER_SPEED_ITEM->number.value];
		}
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_STEPS_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_STEPS
		indigo_property_copy_values(FOCUSER_STEPS_PROPERTY, property, false);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		FOCUSER_STEPS_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_STEPS_PROPERTY, NULL);
		if (FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM->sw.value)
			FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value + FOCUSER_STEPS_ITEM->number.value;
		else
			FOCUSER_POSITION_ITEM->number.target = FOCUSER_POSITION_ITEM->number.value - FOCUSER_STEPS_ITEM->number.value;
		[delegate command:CMD_SPOS index:0 value:FOCUSER_POSITION_ITEM->number.target];
		[delegate command:CMD_GPOS index:0 value:0];
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_POSITION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_POSITION
		indigo_property_copy_values(FOCUSER_POSITION_PROPERTY, property, false);
		FOCUSER_POSITION_PROPERTY->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, FOCUSER_POSITION_PROPERTY, NULL);
		[delegate command:CMD_SPOS index:0 value:FOCUSER_POSITION_ITEM->number.target];
		[delegate command:CMD_GPOS index:0 value:0];
		return INDIGO_OK;
	} else if (indigo_property_match(FOCUSER_ABORT_MOTION_PROPERTY, property)) {
		// -------------------------------------------------------------------------------- FOCUSER_ABORT_MOTION
		indigo_property_copy_values(FOCUSER_ABORT_MOTION_PROPERTY, property, false);
		if (FOCUSER_ABORT_MOTION_ITEM->sw.value) {
			FOCUSER_ABORT_MOTION_ITEM->sw.value = false;
			FOCUSER_ABORT_MOTION_PROPERTY->state = INDIGO_BUSY_STATE;
			indigo_update_property(device, FOCUSER_ABORT_MOTION_PROPERTY, NULL);
			[delegate command:CMD_STOP index:0 value:0];
		}
		return INDIGO_OK;
	// --------------------------------------------------------------------------------
	}
	return indigo_focuser_change_property(device, client, property);
}

static indigo_result focuser_detach(indigo_device *device) {
	assert(device != NULL);
	if (CONNECTION_CONNECTED_ITEM->sw.value)
		indigo_device_disconnect(NULL, device->name);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_focuser_detach(device);
}

// --------------------------------------------------------------------------------

indigo_result indigo_focuser_mjkzz_bt(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	
	SET_DRIVER_INFO(info, FOCUSER_MJKZZ_BT_NAME, __FUNCTION__, DRIVER_VERSION, last_action);

	if (action == last_action)
		return INDIGO_OK;

	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			delegate = [[MJKZZBTDelegate alloc] init];
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

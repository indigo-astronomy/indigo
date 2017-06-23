// Copyright (c) 2017 CloudMakers, s. r. o.
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

/** INDIGO ICA PTP wrapper debug support
 \file indigo_ica_ptp_debug.m
 */


#import <Foundation/Foundation.h>

#import "indigo_ica_ptp.h"

//---------------------------------------------------------------------------------------------------------- PTPOperationRequest

@implementation PTPOperationRequest(PTPDebug)

- (NSString*)description {
	NSMutableString* s = [NSMutableString stringWithFormat:@"%@ 0x%04x params = [", [self class], self.operationCode];
	if (self.numberOfParameters > 0)
		[s appendFormat:@"0x%08X", self.parameter1];
	if (self.numberOfParameters > 1)
		[s appendFormat:@", 0x%08X", self.parameter2];
	if (self.numberOfParameters > 2)
		[s appendFormat:@", 0x%08X", self.parameter3];
	if (self.numberOfParameters > 3)
		[s appendFormat:@", 0x%08X", self.parameter4];
	if (self.numberOfParameters > 4)
		[s appendFormat:@", 0x%08X", self.parameter5];
	[s appendString:@"]"];
	return s;
}

@end

//--------------------------------------------------------------------------------------------------------- PTPOperationResponse

@implementation PTPOperationResponse(PTPDebug)

- (NSString*)description {
	NSMutableString* s = [NSMutableString stringWithFormat:@"%@ 0x%04x params = [", [self class], self.responseCode];
	if (self.numberOfParameters > 0)
		[s appendFormat:@"0x%08X", self.parameter1];
	if (self.numberOfParameters > 1)
		[s appendFormat:@", 0x%08X", self.parameter2];
	if (self.numberOfParameters > 2)
		[s appendFormat:@", 0x%08X", self.parameter3];
	if (self.numberOfParameters > 3)
		[s appendFormat:@", 0x%08X", self.parameter4];
	if (self.numberOfParameters > 4)
		[s appendFormat:@", 0x%08X", self.parameter5];
	[s appendString:@"]"];
	return s;
}

@end

//--------------------------------------------------------------------------------------------------------------------- PTPEvent

@implementation PTPEvent(PTPDebug)

- (NSString*)description {
	NSMutableString* s = [NSMutableString stringWithFormat:@"%@ 0x%04x params = [", [self class], self.eventCode];
	if (self.numberOfParameters > 0)
		[s appendFormat:@"0x%08X", self.parameter1];
	if (self.numberOfParameters > 1)
		[s appendFormat:@", 0x%08X", self.parameter2];
	if (self.numberOfParameters > 2)
		[s appendFormat:@", 0x%08X", self.parameter3];
	[s appendString:@"]"];
	return s;
}


@end

//------------------------------------------------------------------------------------------------------------------------------ PTPProperty

@implementation PTPProperty(PTPDebug)

- (NSString*)description {
	NSMutableString* s = [NSMutableString stringWithFormat:@"%@ 0x%04x 0x%04x %@", [self class], self.propertyCode, self.type, self.readOnly ? @"ro" : @"rw"];
	if (self.min)
		[s appendFormat:@", min = %@", self.min];
	if (self.max)
		[s appendFormat:@", max = %@", self.max];
	if (self.step)
		[s appendFormat:@", step = %@", self.step];
	if (self.supportedValues) {
		[s appendFormat:@", values = [%@", self.supportedValues.firstObject];
		for (int i = 1; i < self.supportedValues.count; i++)
			[s appendFormat:@", %@", [self.supportedValues objectAtIndex:i]];
		[s appendString:@"]"];
	}
	[s appendFormat:@", default = %@", self.defaultValue];
	[s appendFormat:@", value = %@", self.value];
	return s;
}

@end

//--------------------------------------------------------------------------------------------------------------------- PTPDeviceInfo

@implementation PTPDeviceInfo(PTPDebug)

- (NSString*)debug {
	return [NSString stringWithFormat:@"%@ %@ (%@), PTP %.2f, PTP EXT 0x%08x (%.2f)", [self class], self.model, self.version, self.standardVersion / 100.0, self.vendorExtensionID, self.vendorExtensionVersion / 100.0];
}

@end

//------------------------------------------------------------------------------------------------------------------------------

@implementation ICCameraDevice(PTPDebug)

- (void)dumpData:(void*)data length:(int)length comment:(NSString*)comment {
	UInt32	i, j;
	UInt8*  p;
	char	fStr[80];
	char*   fStrP = NULL;
	NSMutableString*  s = [NSMutableString stringWithFormat:@"\n  %@ [%d bytes]:\n\n", comment, length];
	p = (UInt8*)data;
	for ( i = 0; i < length; i++ ) {
		if ( (i % 16) == 0 ) {
			fStrP = fStr;
			fStrP += snprintf( fStrP, 10, "    %4X:", (unsigned int)i );
		}
		if ( (i % 4) == 0 )
			fStrP += snprintf( fStrP, 2, " " );
		fStrP += snprintf( fStrP, 3, "%02X", (UInt8)(*(p+i)) );
		if ( (i % 16) == 15 ) {
			fStrP += snprintf( fStrP, 2, " " );
			for ( j = i-15; j <= i; j++ ) {
				if ( *(p+j) < 32 || *(p+j) > 126 )
					fStrP += snprintf( fStrP, 2, "." );
				else
					fStrP += snprintf( fStrP, 2, "%c", *(p+j) );
			}
			[s appendFormat:@"%s\n", fStr];
		}
		if ( (i % 256) == 255 )
			[s appendString:@"\n"];
	}
	if ( (i % 16) ) {
		for ( j = (i % 16); j < 16; j ++ )
			{
			fStrP += snprintf( fStrP, 3, "  " );
			if ( (j % 4) == 0 )
				fStrP += snprintf( fStrP, 2, " " );
			}
		fStrP += snprintf( fStrP, 2, " " );
		for ( j = i - (i % 16 ); j < length; j++ ) {
			if ( *(p+j) < 32 || *(p+j) > 126 )
				fStrP += snprintf( fStrP, 2, "." );
			else
				fStrP += snprintf( fStrP, 2, "%c", *(p+j) );
		}
		for ( j = (i % 16); j < 16; j ++ ) {
			fStrP += snprintf( fStrP, 2, " " );
		}
		[s appendFormat:@"%s\n", fStr];
	}
	[s appendString:@"\n"];
	NSLog(@"%@", s);
}

@end


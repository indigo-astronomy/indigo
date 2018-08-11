/*
 * Copyright (c) 2007 Dave Dribin, Lucas Newman
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#import "DDHidKeyboardBarcodeScanner.h"
#import "DDHidElement.h"
#import "DDHidUsage.h"
#import "DDHidQueue.h"
#import "DDHidEvent.h"
#include <IOKit/hid/IOHIDUsageTables.h>

@interface DDHidKeyboardBarcodeScanner (DDHidKeyboardBarcodeDelegate)

- (void) ddhidKeyboardBarcodeScanner: (DDHidKeyboardBarcodeScanner *) keyboardBarcodeScanner
                          gotBarcode: (NSString *) barcode;

@end

@interface DDHidKeyboardBarcodeScanner (Private)

- (void) initKeyboardElements: (NSArray *) elements;
- (void) ddhidQueueHasEvents: (DDHidQueue *) hidQueue;
- (void) processBarcodeDigit: (unsigned) usageId;
- (void) clearAccumulatedInput;
- (void) invalidateBarcodeInputTimer;

@end

@implementation DDHidKeyboardBarcodeScanner

+ (NSArray *) allPossibleKeyboardBarcodeScanners;
{
    return [DDHidDevice allDevicesMatchingUsagePage: kHIDPage_GenericDesktop
                                            usageId: kHIDUsage_GD_Keyboard
                                          withClass: self
                                  skipZeroLocations: YES];
}

- (id) initWithDevice: (io_object_t) device error: (NSError **) error_;
{
    self = [super initWithDevice: device error: error_];
    if (self == nil)
        return nil;
    
    mKeyElements = [[NSMutableArray alloc] init];
    mAccumulatedDigits = [[NSMutableString alloc] init];
    mBarcodeInputTimer = nil;
    
    if ([[self productName] rangeOfString:@"Apple"].location != NSNotFound || [[self productName] rangeOfString:@"Internal"].location != NSNotFound)
        mIsLikelyKeyboardBarcodeScanner = NO;
    else
        mIsLikelyKeyboardBarcodeScanner = YES; // if we see invalid barcodes, we can change our mind
    
    [self initKeyboardElements: [self elements]];
    
    return self;
}

//=========================================================== 
// dealloc
//=========================================================== 
- (void) dealloc
{
    [self invalidateBarcodeInputTimer];
    
    mKeyElements = nil;
    mAccumulatedDigits = nil;
}

#pragma mark -
#pragma mark Keyboard Elements

- (NSArray *) keyElements;
{
    return mKeyElements;
}

- (unsigned) numberOfKeys;
{
    return [mKeyElements count];
}

- (void) addElementsToQueue: (DDHidQueue *) queue;
{
    [queue addElements: mKeyElements];
}

#pragma mark -
#pragma mark Asynchronous Notification

- (void) setDelegate: (id) delegate;
{
    mDelegate = delegate;
}

- (void) addElementsToDefaultQueue;
{
    [self addElementsToQueue: mDefaultQueue];
}

#pragma mark -
#pragma mark Properties

- (BOOL) isLikelyKeyboardBarcodeScanner;
{
    return mIsLikelyKeyboardBarcodeScanner;
}

@end

@implementation DDHidKeyboardBarcodeScanner (DDHidKeyboardDelegate)

- (void) ddhidKeyboardBarcodeScanner: (DDHidKeyboardBarcodeScanner *) keyboardBarcodeScanner
                          gotBarcode: (NSString *) barcode;
{
    if ([mDelegate respondsToSelector: _cmd])
        [mDelegate ddhidKeyboardBarcodeScanner: keyboardBarcodeScanner gotBarcode: barcode];
}

@end

@implementation DDHidKeyboardBarcodeScanner (Private)

- (void) initKeyboardElements: (NSArray *) elements;
{
    NSEnumerator * e = [elements objectEnumerator];
    DDHidElement * element;
    while ((element = [e nextObject]))
    {
        unsigned usagePage = [[element usage] usagePage];
        unsigned usageId = [[element usage] usageId];
        if (usagePage == kHIDPage_KeyboardOrKeypad)
        {
            if ((usageId >= kHIDUsage_KeyboardA) && (usageId <= kHIDUsage_Keyboard0))
            {
                [mKeyElements addObject: element];
            }
        }
        NSArray * subElements = [element elements];
        if (subElements != nil)
            [self initKeyboardElements: subElements];
    }
}

- (void) ddhidQueueHasEvents: (DDHidQueue *) hidQueue;
{
    DDHidEvent * event;
    while ((event = [hidQueue nextEvent]))
    {
        DDHidElement * element = [self elementForCookie: [event elementCookie]];
        unsigned usageId = [[element usage] usageId];
        SInt32 value = [event value];
        if (value == 1) // key down
            [self processBarcodeDigit: usageId];
    }
}

#define UPC_A_BARCODE_LENGTH (12)
#define BARCODE_INPUT_TIMEOUT (0.5)
   
- (void) processBarcodeDigit: (unsigned) usageId;
{
    if (usageId <= kHIDUsage_KeyboardZ || usageId >= kHIDUsage_KeyboardCapsLock) { // an alphabetic key was pressed => probably not a barcode scanner
        [self willChangeValueForKey:@"isLikelyKeyboardBarcodeScanner"];
        mIsLikelyKeyboardBarcodeScanner = NO;
        [self didChangeValueForKey:@"isLikelyKeyboardBarcodeScanner"];
        
        [self clearAccumulatedInput];
        return;
    }
    
    if (!mBarcodeInputTimer) // schedule a timer to make sure we get the rest of the digits in a timely manner
        mBarcodeInputTimer = [NSTimer scheduledTimerWithTimeInterval:BARCODE_INPUT_TIMEOUT target:self selector:@selector(fireBarcodeInputTimeout:) userInfo:nil repeats:NO];
    
    [mAccumulatedDigits appendString:[NSString stringWithFormat:@"%d", (usageId + 1) % 10]];
}

- (void) fireBarcodeInputTimeout: (NSTimer *) timer;
{
    if ([mAccumulatedDigits length] >= UPC_A_BARCODE_LENGTH)
        [self ddhidKeyboardBarcodeScanner: self gotBarcode: [mAccumulatedDigits copy]];
    [self clearAccumulatedInput];
}

- (void) clearAccumulatedInput;
{
    [mAccumulatedDigits deleteCharactersInRange:NSMakeRange(0, [mAccumulatedDigits length])];
    
    [self invalidateBarcodeInputTimer];
}

- (void) invalidateBarcodeInputTimer;
{
    [mBarcodeInputTimer invalidate];
    mBarcodeInputTimer = nil;
}

@end

//
//  indigo_ica_nikon.h
//  IndigoApps
//
//  Created by Peter Polakovic on 11/07/2017.
//  Copyright © 2017 CloudMakers, s. r. o. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "indigo_ica_ptp.h"

@interface PTPNikonCamera : PTPCamera

-(id)initWithICCamera:(ICCameraDevice *)icCamera delegate:(NSObject<PTPDelegateProtocol> *)delegate;

@end

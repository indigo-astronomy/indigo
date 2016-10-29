//  Copyright (c) 2016 CloudMakers, s. r. o.
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above
//  copyright notice, this list of conditions and the following
//  disclaimer in the documentation and/or other materials provided
//  with the distribution.
//
//  3. The name of the author may not be used to endorse or promote
//  products derived from this software without specific prior
//  written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE AUTHOR 'AS IS' AND ANY EXPRESS
//  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
//  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
//  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
//  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//  version history
//  2.0 Build 0 - PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

import Cocoa
import ServiceManagement

func serverCallback(count: Int32) {
  NSLog("%d clients", count)
}

@NSApplicationMain class AppDelegate: NSObject, NSApplicationDelegate, NetServiceDelegate {

  @IBOutlet weak var window: NSWindow!
	
  func netServiceWillPublish(_ sender: NetService) {
    NSLog("Bonjour service is ready to publish.")
  }
  
  func netServiceDidPublish(_ sender: NetService) {
    NSLog("Bonjour service was successfully published.")
  }
  
  func netService(_ sender: NetService, didNotPublish errorDict: [String : NSNumber]) {
    NSLog("Bonjour service could not be published.");
    NSLog("\(errorDict)");
  }
  
  func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
    return true
  }

	let serverId = "eu.cloudmakers.indi.indigo_server"
	let service = NetService(domain: "", type: "_indi._tcp", name: "", port: 7624)
	
  func applicationDidFinishLaunching(_ notification: Notification) {
		var error: Unmanaged<CFError>? = nil
		if SMJobRemove(kSMDomainUserLaunchd, serverId as CFString, nil, false, &error)  {
			if let executable = Bundle.main.path(forAuxiliaryExecutable: "indigo_server") {
				let plist: [String:Any] = [ "Label": serverId, "KeepAlive": true, "Program": executable]
				if SMJobSubmit(kSMDomainUserLaunchd, plist as CFDictionary, nil, &error) {				
					NSLog("Server job was successfully installed!")
					service.delegate = self
					service.publish()
				} else {
					NSLog("Failed to install server job! \(error)")
				}
			}
		} else {
			NSLog("Failed to remove server job! \(error)")
		}
  }
	
	func applicationWillTerminate(_ notification: Notification) {
		var error: Unmanaged<CFError>? = nil
		if SMJobRemove(kSMDomainUserLaunchd, serverId as CFString, nil, false, &error)  {
			NSLog("Server job was successfully removed!")
		} else {
			NSLog("Failed to remove server job! \(error)")
		}
	}
}


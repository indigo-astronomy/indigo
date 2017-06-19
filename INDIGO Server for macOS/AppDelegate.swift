// Copyright (c) 2016 CloudMakers, s. r. o.
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

import Cocoa
import ServiceManagement
import WebKit

func serverCallback(count: Int32) {
  NSLog("%d clients", count)
}

@NSApplicationMain class AppDelegate: NSObject, NSApplicationDelegate, NetServiceDelegate {

  @IBOutlet weak var window: NSWindow!
	@IBOutlet weak var web: WebView!
	@IBOutlet weak var status: NSTextField!
	
  func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
    return true
  }

	let serverId = "eu.cloudmakers.indi.indigo_server"
	
  func applicationDidFinishLaunching(_ notification: Notification) {
		var error: Unmanaged<CFError>? = nil
		SMJobRemove(kSMDomainUserLaunchd, serverId as CFString, nil, false, &error)
		if let executable = Bundle.main.path(forAuxiliaryExecutable: "indigo_server") {
			let arguments: [String] = [ executable, "--", "-l", "-s" ]
			let plist: [String:Any] = [ "Label": serverId, "KeepAlive": true, "Program": executable, "ProgramArguments": arguments]
			if SMJobSubmit(kSMDomainUserLaunchd, plist as CFDictionary, nil, &error) {				
				status.stringValue = "Server job was successfully installed!"
				DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
					self.web.mainFrameURL = "http://localhost:7624/ctrl"
				}
			} else {
				status.stringValue = "Failed to install server job! \(error)"
			}
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


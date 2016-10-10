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
//  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
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
//  0.0 PoC by Peter Polakovic <peter.polakovic@cloudmakers.eu>

import Cocoa

func serverCallback(count: Int32) {
  NSLog("%d clients", count)
}

@NSApplicationMain class AppDelegate: NSObject, NSApplicationDelegate, NetServiceDelegate {

  @IBOutlet weak var window: NSWindow!
  
  var thread: Thread?
  
  func netServiceWillPublish(_ sender: NetService) {
    NSLog("INDIGO Service is ready to publish.")
  }
  
  func netServiceDidPublish(_ sender: NetService) {
    NSLog("INDIGO Service was successfully published.")
  }
  
  func netService(_ sender: NetService, didNotPublish errorDict: [String : NSNumber]) {
    NSLog("INDIGO Service  could not be published.");
    NSLog("%@", errorDict);
  }
  
  func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
    return true
  }

  func server() {
    let service = NetService(domain: "", type: "_indi._tcp", name: "", port: 7624)
    service.delegate = self
    service.publish()
    indigo_start()
    indigo_attach_device(indigo_ccd_simulator())
    indigo_ccd_sx_register()
    indigo_server_xml(serverCallback)
  }
  
  func applicationDidFinishLaunching(_ notification: Notification) {
    thread = Thread(target: self, selector: #selector(server), object: nil)
    if let thread = thread {
      thread.start();
    }
  }
}


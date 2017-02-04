//
// ASCOM Driver for INDIGO
//
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

using System;
using ASCOM.Utilities;
using INDIGO;
using System.Collections;
using System.Windows.Forms;

namespace ASCOM.INDIGO {
  public class BaseDriver {
    protected bool connectedState;
    protected Util utilities;

    protected WaitForForm waitFor = new WaitForForm();
    protected bool waitingForDevice;
    protected bool waitingForConnected;
    protected bool waitingForDisconnected;

    public static string deviceNameProfileName = "INDIGO Device";

    public Client client;
    public Device device;
    public string deviceName;
    public Device.InterfaceMask deviceInterface;

    virtual protected void propertyChanged(Property property) {
      //Console.WriteLine("Property \"" + property.DeviceName + "\" \"" + property.Name + "\"");
      if (property.DeviceName == deviceName && property.Name == "CONNECTION" && property.State == Property.States.Ok) {
        waitFor.Hide(ref waitingForDevice);
        SwitchItem item = (SwitchItem)property.GetItem("CONNECTED");
        connectedState = item != null && item.Value;
        if (connectedState) {
          waitFor.Hide(ref waitingForConnected);
        } else if (!connectedState) {
          waitFor.Hide(ref waitingForDisconnected);
        }
      }
    }

    private void propertyAdded(Property property) {
      propertyChanged(property);
    }

    private void propertyUpdated(Property property) {
      propertyChanged(property);
    }

    private void propertyRemoved(Property property) {
    }

    private void deviceAdded(Device device) {
      device.PropertyAdded += propertyAdded;
      device.PropertyUpdated += propertyUpdated;
      device.PropertyRemoved += propertyRemoved;
      if (device.Name == deviceName) {
        this.device = device;
        Console.WriteLine("Device \"" + device.Name + "\" found on \"" + device.Server.Name + "\"");
      }
    }

    private void deviceRemoved(Device device) {
      if (this.device == device) {
        this.device = null;
        connectedState = false;
        Console.WriteLine("Device \"" + device.Name + "\" lost");
      }
    }

    private void serverAdded(Server server) {
      server.DeviceAdded += deviceAdded;
      server.DeviceRemoved += deviceRemoved;
    }

    private void serverRemoved(Server server) {
    }

    protected BaseDriver() {
      client = new Client();
      client.ServerAdded += serverAdded;
      client.ServerRemoved += serverRemoved;
      connectedState = false;
      utilities = new Util();
      ReadProfile();
    }

    protected bool IsConnected {
      get {
        if (device != null)
          return connectedState;
        return false;
      }
    }

    protected void CheckConnected(string message) {
      if (!IsConnected) {
        throw new ASCOM.NotConnectedException(message);
      }
    }

    virtual protected void WriteProfile() {
    }

    virtual protected void ReadProfile() {
    }

    public bool Connected {
      get {
        return IsConnected;
      }
      set {
        if (value == IsConnected)
          return;

        if (value) {
          if (device == null) {
            waitFor.Wait(out waitingForDevice, "Waiting for \"" + deviceName + "\"");
          }
          if (device == null) {
            Console.WriteLine("Can't connect to \"" + deviceName + "\"");
          } else {
            if (!IsConnected) {
              device.Connect();
              waitFor.Wait(out waitingForConnected, "Connecting to \"" + deviceName + "\"");
            }
            Console.WriteLine("Connected to \"" + deviceName + "\"");
          }
        } else {
          device.Disconnect();
          waitFor.Wait(out waitingForDisconnected, "Disconnecting from \"" + deviceName + "\"");
          Console.WriteLine("Disconnected from \"" + deviceName + "\"");
        }
      }
    }

    public void SetupDialog() {
      if (IsConnected)
        System.Windows.Forms.MessageBox.Show("Device is connected");
      else {
        using (DeviceSelectionForm F = new DeviceSelectionForm(this, string.Empty)) {
          var result = F.ShowDialog();
          if (result == System.Windows.Forms.DialogResult.OK) {
            WriteProfile();
            Console.WriteLine("Device \"" + device.Name + "\" on \"" + device.Server.Name + "\" selected");
          }
          F.Dispose();
        }
      }
    }

    public void CommandBlind(string command, bool raw) {
      throw new ASCOM.MethodNotImplementedException("CommandBlind");
    }

    public bool CommandBool(string command, bool raw) {
      throw new ASCOM.MethodNotImplementedException("CommandBool");
    }

    public string CommandString(string command, bool raw) {
      throw new ASCOM.MethodNotImplementedException("CommandString");
    }

    public ArrayList SupportedActions {
      get {
        return new ArrayList();
      }
    }

    public string Action(string actionName, string actionParameters) {
      throw new ASCOM.ActionNotImplementedException("Action " + actionName + " is not implemented by this driver");
    }

    public void Dispose() {
      client.Dispose();
      client = null;
      utilities.Dispose();
      utilities = null;
    }
  }
}

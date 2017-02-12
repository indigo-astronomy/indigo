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
  public class BaseDriver : Indigo {
    protected bool connectedState;
    protected Util utilities;

    protected WaitForForm waitFor = new WaitForForm();
    protected bool waitingForDevice;
    protected bool waitingForConfigProperty;
    protected bool waitingForConnectionProperty;
    protected bool waitingForConnected;
    protected bool waitingForDisconnected;

    public static string deviceNameProfileName = "INDIGO Device";

    public static Client client = null;
    public Device device;
    public string deviceName;
    public Device.InterfaceMask deviceInterface;

    private SwitchProperty configProperty, connectionProperty;

    virtual protected void propertyChanged(Property property) {
      //Log("Property \"" + property.DeviceName + "\" \"" + property.Name + "\"");
    }

    virtual protected void propertyAdded(Property property) {
      if (property.DeviceName == deviceName) {
        Log("def" + property);
        if (property.Name == "CONFIG") {
          configProperty = (SwitchProperty)property;
          waitFor.Hide(ref waitingForConfigProperty);
        } else if (property.Name == "CONNECTION") {
          connectionProperty = (SwitchProperty)property;
          waitFor.Hide(ref waitingForConnectionProperty);
        }
      }
      propertyChanged(property);
    }

    virtual protected void propertyUpdated(Property property) {
      if (property.DeviceName == deviceName) {
        Log("set" + property);
        if (property == connectionProperty) {
          SwitchItem item = (SwitchItem)property.GetItem("CONNECTED");
          connectedState = item != null && item.Value;
          if (connectedState) {
            waitFor.Hide(ref waitingForConnected);
          } else if (!connectedState) {
            waitFor.Hide(ref waitingForDisconnected);
          }
        }
      }
      propertyChanged(property);
    }

    private void propertyRemoved(Property property) {
      Log("del" + property);
      if (property.DeviceName == deviceName && property.Name == "CONFIG") {
        connectionProperty = null;
      } else if (property.DeviceName == deviceName && property.Name == "CONNECTION") {
        connectionProperty = null;
      }
    }

    private void deviceAdded(Device device) {
      device.PropertyAdded += propertyAdded;
      device.PropertyUpdated += propertyUpdated;
      device.PropertyRemoved += propertyRemoved;
      if (device.Name == deviceName) {
        this.device = device;
        waitFor.Hide(ref waitingForDevice);
        Log("Device \"" + device.Name + "\" found on \"" + device.Server.Name + "\"");
      }
      foreach (Property property in device.Properties)
        propertyAdded(property);
    }

    private void deviceRemoved(Device device) {
      if (this.device == device) {
        this.device = null;
        connectedState = false;
        Log("Device \"" + device.Name + "\" lost");
      }
    }

    private void serverAdded(Server server) {
      server.DeviceAdded += deviceAdded;
      server.DeviceRemoved += deviceRemoved;
      foreach (Device device in server.Devices)
        deviceAdded(device);
    }

    private void serverRemoved(Server server) {
    }

    protected BaseDriver() {
      if (client == null)
        client = new Client();
      client.Mutex.WaitOne();
      foreach (Server server in client.Servers)
        serverAdded(server);
      client.Mutex.ReleaseMutex();
      client.ServerAdded += serverAdded;
      client.ServerRemoved += serverRemoved;
      connectedState = false;
      utilities = new Util();
      ReadProfile();
    }

    public void Dispose() {
      client.ServerAdded -= serverAdded;
      client.ServerRemoved -= serverRemoved;
      foreach (Server server in client.Servers) {
        server.DeviceAdded -= deviceAdded;
        server.DeviceRemoved -= deviceRemoved;
        foreach (Device device in server.Devices) {
          device.PropertyAdded -= propertyAdded;
          device.PropertyUpdated -= propertyUpdated;
          device.PropertyRemoved -= propertyRemoved;
        }
      }
      utilities.Dispose();
      utilities = null;
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
            if (deviceName == string.Empty)
              SetupDialog();
            else {
              device = client.GetDevice(deviceName);
              if (device == null)
                waitFor.Wait(out waitingForDevice, "Waiting for \"" + deviceName + "\"", this);
            }
          }
          if (device == null) {
            Log("Can't connect to \"" + deviceName + "\"");
          } else {
            configProperty = (SwitchProperty)device.GetProperty("CONFIG");
            if (configProperty == null)
              waitFor.Wait(out waitingForConfigProperty, "Waiting for CONFIG property", this);
            if (configProperty != null)
              configProperty.SetSingleValue("LOAD", true);
            connectionProperty = (SwitchProperty)device.GetProperty("CONNECTION");
            if (connectionProperty == null)
              waitFor.Wait(out waitingForConnectionProperty, "Waiting for CONNECTION property", this);
            if (!IsConnected) {
              client.Mutex.WaitOne();
              foreach (Property property in device.Properties)
                propertyAdded(property);
              client.Mutex.ReleaseMutex();
              connectionProperty.SetSingleValue("CONNECTED", true);
              waitFor.Wait(out waitingForConnected, "Connecting to \"" + deviceName + "\"", this);
            }
            Log("Connected to \"" + deviceName + "\"");
          }
        } else {
          if (connectionProperty != null) {
            connectionProperty.SetSingleValue("DISCONNECTED", true);
          }
          device = null;
          connectedState = false;
          Log("Disconnected from \"" + deviceName + "\"");
        }
      }
    }

    public void SetupDialog() {
      if (IsConnected)
        Connected = false;
      using (DeviceSelectionForm F = new DeviceSelectionForm(this)) {
        var result = F.ShowDialog();
        if (result == System.Windows.Forms.DialogResult.OK) {
          WriteProfile();
          Log("Device \"" + device.Name + "\" on \"" + device.Server.Name + "\" selected");
        }
        F.Dispose();
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
  }
}

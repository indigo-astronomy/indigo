using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using ASCOM.Utilities;
using INDIGO;

namespace ASCOM.INDIGO {
  public class BaseDriver {
    protected bool connectedState;
    protected Util utilities;

    public static string indigoDeviceProfileName = "INDIGO Device";
    public static string indigoDeviceDefault = "";

    public Client client;
    public Device device;
    public string deviceName;
    public Device.InterfaceMask deviceInterface;

    private void propertyAdded(Property property) {
    }

    private void propertyUpdated(Property property) {
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
            using (DeviceSelectionForm F = new DeviceSelectionForm(this, deviceName)) {
              var result = F.ShowDialog();
              if (result == System.Windows.Forms.DialogResult.OK) {
                WriteProfile();
                Console.WriteLine("Device \"" + device.Name + "\" on \"" + device.Server.Name + "\" selected");
              }
              F.Dispose();
            }
          }
          if (device == null) {
            Console.WriteLine("Can't connect to " + deviceName);
          } else {
            connectedState = true;
            Console.WriteLine("Connecting to " + deviceName);
            device.Connect();
          }
        } else {
          connectedState = false;
          Console.WriteLine("Disconnecting from port " + deviceName);
          device.Disconnect();
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

    public void Dispose() {
      client.Dispose();
      client = null;
      utilities.Dispose();
      utilities = null;
    }
  }
}

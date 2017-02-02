//tabs=4
// --------------------------------------------------------------------------------
//
// ASCOM FilterWheel driver for INDIGO
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
//
// Author:		(PPO) Peter Polakovic, CloudMakers, s. r. o. <peter.polakovic@cloumakers.eu>
//
// Date			Who	Vers	Description
// -----------	---	-----	-------------------------------------------------------
// 21-jan-2017	PPO	6.0.0	Initial edit, created from ASCOM driver template
// --------------------------------------------------------------------------------
//

#define FilterWheel

using System;
using System.Runtime.InteropServices;
using ASCOM.Utilities;
using ASCOM.DeviceInterface;
using System.Globalization;
using System.Collections;
using INDIGO;
using System.Threading;

namespace ASCOM.INDIGO {

  [Guid("fd8e1019-78d3-4b3b-9eae-08a76af88b4d")]
  [ClassInterface(ClassInterfaceType.None)]
  public class FilterWheel : IFilterWheelV2 {
    internal static string driverID = "ASCOM.INDIGO.FilterWheel";
    internal static string driverDescription = "INDIGO FilterWheel";

    internal string indigoDeviceProfileName = "INDIGO Device";
    internal string indigoDeviceDefault = "";

    internal string traceStateProfileName = "Trace Level";
    internal string traceStateDefault = "true";

    internal string indigoDevice;
    internal bool traceState = true;

    private bool connectedState;
    private Util utilities;
    private TraceLogger tl;

    internal Client client;
    internal Device device;
    internal bool connecting = false;

    private Property connectionProperty;

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
      if (device.Name == indigoDevice) {
        this.device = device;
        tl.LogMessage("deviceAdded", "Device \"" + device.Name + "\" found on \"" + device.Server.Name + "\"");
      }
    }

    private void deviceRemoved(Device device) {
      if (this.device == device) {
        this.device = null;
        connectedState = false;
        tl.LogMessage("deviceRemoved", "Device \"" + device.Name + "\" lost");
      }
    }

    private void serverAdded(Server server) {
      server.DeviceAdded += deviceAdded;
      server.DeviceRemoved += deviceRemoved;
    }

    private void serverRemoved(Server server) {
    }

    public FilterWheel() {
      ReadProfile();

      tl = new TraceLogger("", "INDIGO");
      tl.Enabled = traceState;
      tl.LogMessage("FilterWheel", "Starting initialisation");

      connectedState = false;
      utilities = new Util();
      client = new Client();
      client.ServerAdded += serverAdded;
      client.ServerRemoved += serverRemoved;

      //TODO: Implement your additional construction here

      tl.LogMessage("FilterWheel", "Completed initialisation");
    }

    #region Common properties and methods.

    public void SetupDialog() {
      if (IsConnected)
        System.Windows.Forms.MessageBox.Show("Device is connected");
      else {
        using (SetupDialogForm F = new SetupDialogForm(this)) {
          var result = F.ShowDialog();
          if (result == System.Windows.Forms.DialogResult.OK) {
            WriteProfile();
            tl.LogMessage("SetupDialog", "Device \"" + device.Name + "\" on \"" + device.Server.Name + "\" selected");
          }
          F.Dispose();
        }
      }
    }

    public ArrayList SupportedActions {
      get {
        return new ArrayList();
      }
    }

    public string Action(string actionName, string actionParameters) {
      throw new ASCOM.ActionNotImplementedException("Action " + actionName + " is not implemented by this driver");
    }

    public void CommandBlind(string command, bool raw) {
      CheckConnected("CommandBlind");
      // TODO
    }

    public bool CommandBool(string command, bool raw) {
      throw new ASCOM.MethodNotImplementedException("CommandBool");
    }

    public string CommandString(string command, bool raw) {
      throw new ASCOM.MethodNotImplementedException("CommandString");
    }

    public void Dispose() {
      tl.Enabled = false;
      tl.Dispose();
      tl = null;
      client.Dispose();
      client = null;
      utilities.Dispose();
      utilities = null;
    }

    public bool Connected {
      get {
        tl.LogMessage("Connected Get", IsConnected.ToString());
        return IsConnected;
      }
      set {
        tl.LogMessage("Connected Set", value.ToString());
        if (value == IsConnected)
          return;

        if (value) {
          if (device == null) {
            connecting = true;
            SetupDialog();
            connecting = false;
          }
          if (device == null) {
            tl.LogMessage("Connected Set", "Can't connect to " + indigoDevice);
          } else {
            connectedState = true;
            tl.LogMessage("Connected Set", "Connecting to " + indigoDevice);
            device.Connect();
          }
        } else {
          connectedState = false;
          tl.LogMessage("Connected Set", "Disconnecting from port " + indigoDevice);
          device.Disconnect();
        }
      }
    }

    public string Description {
      get {
        tl.LogMessage("Description Get", driverDescription);
        return driverDescription;
      }
    }

    public string DriverInfo {
      get {
        Version version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
        string driverInfo = "INDIGO FilterWheel driver. Version: " + String.Format(CultureInfo.InvariantCulture, "{0}.{1}", version.Major, version.Minor);
        tl.LogMessage("DriverInfo Get", driverInfo);
        return driverInfo;
      }
    }

    public string DriverVersion {
      get {
        Version version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
        string driverVersion = String.Format(CultureInfo.InvariantCulture, "{0}.{1}", version.Major, version.Minor);
        tl.LogMessage("DriverVersion Get", driverVersion);
        return driverVersion;
      }
    }

    public short InterfaceVersion {
      get {
        tl.LogMessage("InterfaceVersion Get", "2");
        return 2;
      }
    }

    public string Name {
      get {
        string name = "INDIGO FilterWheel";
        tl.LogMessage("Name Get", name);
        return name;
      }
    }

    #endregion

    #region IFilerWheel Implementation
    private int[] fwOffsets = new int[4] { 0, 0, 0, 0 }; //class level variable to hold focus offsets
    private string[] fwNames = new string[4] { "Red", "Green", "Blue", "Clear" }; //class level variable to hold the filter names
    private short fwPosition = 0; // class level variable to retain the current filterwheel position

    public int[] FocusOffsets {
      get {
        foreach (int fwOffset in fwOffsets) {
          tl.LogMessage("FocusOffsets Get", fwOffset.ToString());
        }

        return fwOffsets;
      }
    }

    public string[] Names {
      get {
        foreach (string fwName in fwNames) {
          tl.LogMessage("Names Get", fwName);
        }

        return fwNames;
      }
    }

    public short Position {
      get {
        tl.LogMessage("Position Get", fwPosition.ToString());
        return fwPosition;
      }
      set {
        tl.LogMessage("Position Set", value.ToString());
        if ((value < 0) | (value > fwNames.Length - 1)) {
          tl.LogMessage("", "Throwing InvalidValueException - Position: " + value.ToString() + ", Range: 0 to " + (fwNames.Length - 1).ToString());
          throw new InvalidValueException("Position", value.ToString(), "0 to " + (fwNames.Length - 1).ToString());
        }
        fwPosition = value;
      }
    }

    #endregion

    #region Private properties and methods

    #region ASCOM Registration

    private static void RegUnregASCOM(bool bRegister) {
      using (var P = new ASCOM.Utilities.Profile()) {
        P.DeviceType = "FilterWheel";
        if (bRegister) {
          P.Register(driverID, driverDescription);
        } else {
          P.Unregister(driverID);
        }
      }
    }

    /// <summary>
    /// This function registers the driver with the ASCOM Chooser and
    /// is called automatically whenever this class is registered for COM Interop.
    /// </summary>
    /// <param name="t">Type of the class being registered, not used.</param>
    /// <remarks>
    /// This method typically runs in two distinct situations:
    /// <list type="numbered">
    /// <item>
    /// In Visual Studio, when the project is successfully built.
    /// For this to work correctly, the option <c>Register for COM Interop</c>
    /// must be enabled in the project settings.
    /// </item>
    /// <item>During setup, when the installer registers the assembly for COM Interop.</item>
    /// </list>
    /// This technique should mean that it is never necessary to manually register a driver with ASCOM.
    /// </remarks>
    [ComRegisterFunction]
    public static void RegisterASCOM(Type t) {
      RegUnregASCOM(true);
    }

    /// <summary>
    /// This function unregisters the driver from the ASCOM Chooser and
    /// is called automatically whenever this class is unregistered from COM Interop.
    /// </summary>
    /// <param name="t">Type of the class being registered, not used.</param>
    /// <remarks>
    /// This method typically runs in two distinct situations:
    /// <list type="numbered">
    /// <item>
    /// In Visual Studio, when the project is cleaned or prior to rebuilding.
    /// For this to work correctly, the option <c>Register for COM Interop</c>
    /// must be enabled in the project settings.
    /// </item>
    /// <item>During uninstall, when the installer unregisters the assembly from COM Interop.</item>
    /// </list>
    /// This technique should mean that it is never necessary to manually unregister a driver from ASCOM.
    /// </remarks>
    [ComUnregisterFunction]
    public static void UnregisterASCOM(Type t) {
      RegUnregASCOM(false);
    }

    #endregion

    private bool IsConnected {
      get {
        // TODO check that the driver hardware connection exists and is connected to the hardware
        return connectedState;
      }
    }

    private void CheckConnected(string message) {
      if (!IsConnected) {
        throw new ASCOM.NotConnectedException(message);
      }
    }

    internal void ReadProfile() {
      using (Profile driverProfile = new Profile()) {
        driverProfile.DeviceType = "FilterWheel";
        indigoDevice = driverProfile.GetValue(driverID, indigoDeviceProfileName, string.Empty, indigoDeviceDefault);
        traceState = Convert.ToBoolean(driverProfile.GetValue(driverID, traceStateProfileName, string.Empty, traceStateDefault));
      }
    }

    internal void WriteProfile() {
      using (Profile driverProfile = new Profile()) {
        driverProfile.DeviceType = "FilterWheel";
        driverProfile.WriteValue(driverID, traceStateProfileName, traceState.ToString());
        driverProfile.WriteValue(driverID, indigoDeviceProfileName, indigoDevice.ToString());
      }
    }

    #endregion
  }
}

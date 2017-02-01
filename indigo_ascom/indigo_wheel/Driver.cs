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

namespace ASCOM.INDIGO {

  /// <summary>
  /// ASCOM FilterWheel Driver for INDIGO.
  /// </summary>
  [Guid("fd8e1019-78d3-4b3b-9eae-08a76af88b4d")]
  [ClassInterface(ClassInterfaceType.None)]
  public class FilterWheel : IFilterWheelV2 {
    /// <summary>
    /// ASCOM DeviceID (COM ProgID) for this driver.
    /// The DeviceID is used by ASCOM applications to load the driver at runtime.
    /// </summary>
    internal static string driverID = "ASCOM.INDIGO.FilterWheel";
    // TODO Change the descriptive string for your driver then remove this line
    /// <summary>
    /// Driver description that displays in the ASCOM Chooser.
    /// </summary>
    private static string driverDescription = "INDIGO FilterWheel";

    internal static string indigoDeviceProfileName = "INDIGO Device";
    internal static string indigoDeviceDefault = "";

    internal static string traceStateProfileName = "Trace Level";
    internal static string traceStateDefault = "false";

    internal static string indigoDevice;
    internal static bool traceState;

    /// <summary>
    /// Private variable to hold the connected state
    /// </summary>
    private bool connectedState;

    /// <summary>
    /// Private variable to hold an ASCOM Utilities object
    /// </summary>
    private Util utilities;

    /// <summary>
    /// Private variable to hold the trace logger object (creates a diagnostic log file with information that you specify)
    /// </summary>
    private TraceLogger tl;

    /// <summary>
    /// Initializes a new instance of the <see cref="INDIGO"/> class.
    /// Must be public for COM registration.
    /// </summary>
    public FilterWheel() {
      ReadProfile();

      tl = new TraceLogger("", "INDIGO");
      tl.Enabled = traceState;
      tl.LogMessage("FilterWheel", "Starting initialisation");

      connectedState = false; // Initialise connected to false
      utilities = new Util(); //Initialise util object

      //TODO: Implement your additional construction here

      tl.LogMessage("FilterWheel", "Completed initialisation");
    }

    #region Common properties and methods.

    /// <summary>
    /// Displays the Setup Dialog form.
    /// If the user clicks the OK button to dismiss the form, then
    /// the new settings are saved, otherwise the old values are reloaded.
    /// THIS IS THE ONLY PLACE WHERE SHOWING USER INTERFACE IS ALLOWED!
    /// </summary>
    public void SetupDialog() {
      if (IsConnected)
        System.Windows.Forms.MessageBox.Show("Already connected, just press OK");

      using (SetupDialogForm F = new SetupDialogForm()) {
        var result = F.ShowDialog();
        if (result == System.Windows.Forms.DialogResult.OK) {
          WriteProfile();
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
      // Clean up the tracelogger and util objects
      tl.Enabled = false;
      tl.Dispose();
      tl = null;
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
          connectedState = true;
          tl.LogMessage("Connected Set", "Connecting to " + indigoDevice);
          // TODO connect to the device
        } else {
          connectedState = false;
          tl.LogMessage("Connected Set", "Disconnecting from port " + indigoDevice);
          // TODO disconnect from the device
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
    // here are some useful properties and methods that can be used as required
    // to help with driver development

    #region ASCOM Registration

    // Register or unregister driver for ASCOM. This is harmless if already
    // registered or unregistered. 
    //
    /// <summary>
    /// Register or unregister the driver with the ASCOM Platform.
    /// This is harmless if the driver is already registered/unregistered.
    /// </summary>
    /// <param name="bRegister">If <c>true</c>, registers the driver, otherwise unregisters it.</param>
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

    /// <summary>
    /// Returns true if there is a valid connection to the driver hardware
    /// </summary>
    private bool IsConnected {
      get {
        // TODO check that the driver hardware connection exists and is connected to the hardware
        return connectedState;
      }
    }

    /// <summary>
    /// Use this function to throw an exception if we aren't connected to the hardware
    /// </summary>
    /// <param name="message"></param>
    private void CheckConnected(string message) {
      if (!IsConnected) {
        throw new ASCOM.NotConnectedException(message);
      }
    }

    /// <summary>
    /// Read the device configuration from the ASCOM Profile store
    /// </summary>
    internal void ReadProfile() {
      using (Profile driverProfile = new Profile()) {
        driverProfile.DeviceType = "FilterWheel";
        indigoDevice = driverProfile.GetValue(driverID, indigoDeviceProfileName, string.Empty, indigoDeviceDefault);
        traceState = Convert.ToBoolean(driverProfile.GetValue(driverID, traceStateProfileName, string.Empty, traceStateDefault));
      }
    }

    /// <summary>
    /// Write the device configuration to the  ASCOM  Profile store
    /// </summary>
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

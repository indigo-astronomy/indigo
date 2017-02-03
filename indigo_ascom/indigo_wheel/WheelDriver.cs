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

namespace ASCOM.INDIGO {

  [Guid("fd8e1019-78d3-4b3b-9eae-08a76af88b4d")]
  [ClassInterface(ClassInterfaceType.None)]
  public class FilterWheel : BaseDriver, IFilterWheelV2 {
    internal static string driverID = "ASCOM.INDIGO.FilterWheel";
    internal static string driverDescription = "INDIGO FilterWheel";


    public FilterWheel() {
      deviceInterface = Device.InterfaceMask.FilterWheel;
    }

    #region Common properties and methods.


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
      client.Dispose();
      client = null;
      utilities.Dispose();
      utilities = null;
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

    public string Description {
      get {
        return driverDescription;
      }
    }

    public string DriverInfo {
      get {
        Version version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
        string driverInfo = "INDIGO FilterWheel driver. Version: " + String.Format(CultureInfo.InvariantCulture, "{0}.{1}", version.Major, version.Minor);
        return driverInfo;
      }
    }

    public string DriverVersion {
      get {
        Version version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
        string driverVersion = String.Format(CultureInfo.InvariantCulture, "{0}.{1}", version.Major, version.Minor);
        return driverVersion;
      }
    }

    public short InterfaceVersion {
      get {
        return 2;
      }
    }

    public string Name {
      get {
        string name = "INDIGO FilterWheel";
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
        return fwOffsets;
      }
    }

    public string[] Names {
      get {
        return fwNames;
      }
    }

    public short Position {
      get {
        return fwPosition;
      }
      set {
        if ((value < 0) | (value > fwNames.Length - 1)) {
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

    [ComRegisterFunction]
    public static void RegisterASCOM(Type t) {
      RegUnregASCOM(true);
    }

    [ComUnregisterFunction]
    public static void UnregisterASCOM(Type t) {
      RegUnregASCOM(false);
    }

    #endregion

    override protected void ReadProfile() {
      using (Profile driverProfile = new Profile()) {
        driverProfile.DeviceType = "FilterWheel";
        deviceName = driverProfile.GetValue(driverID, indigoDeviceProfileName, string.Empty, indigoDeviceDefault);
      }
    }

    override protected void WriteProfile() {
      using (Profile driverProfile = new Profile()) {
        driverProfile.DeviceType = "FilterWheel";
        driverProfile.WriteValue(driverID, indigoDeviceProfileName, deviceName);
      }
    }

    #endregion
  }
}

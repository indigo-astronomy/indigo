//
// ASCOM FilterWheel Driver for INDIGO
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
using System.Runtime.InteropServices;
using System.Globalization;
using ASCOM.Utilities;
using ASCOM.DeviceInterface;
using INDIGO;

namespace ASCOM.INDIGO {

  [Guid("fd8e1019-78d3-4b3b-9eae-08a76af88b4d")]
  [ClassInterface(ClassInterfaceType.None)]
  public class FilterWheel : BaseDriver, IFilterWheelV2 {
    internal static string driverID = "ASCOM.INDIGO.FilterWheel";
    internal static string driverName = "INDIGO FilterWheel";

    public FilterWheel() {
      deviceInterface = Device.InterfaceMask.FilterWheel;
    }

    public string Description {
      get {
        return driverName + " (" + deviceName + ")";
      }
    }

    public string DriverInfo {
      get {
        return Name + " driver, version " + DriverVersion;
      }
    }

    public string DriverVersion {
      get {
        Version version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;
        return String.Format(CultureInfo.InvariantCulture, "{0}.{1}", version.Major, version.Minor);
      }
    }

    public short InterfaceVersion {
      get {
        return 2;
      }
    }

    public string Name {
      get {
        return driverName;
      }
    }

    private int[] offsets = new int[0];
    private string[] names = new string[0];
    private short position = 0;

    public int[] FocusOffsets {
      get {
        return offsets;
      }
    }

    public string[] Names {
      get {
        return names;
      }
    }

    public short Position {
      get {
        return position;
      }
      set {
        if ((value < 0) | (value > names.Length - 1)) {
          throw new InvalidValueException("Position", value.ToString(), "0 to " + (names.Length - 1).ToString());
        }
        position = -1;
        if (IsConnected) {
          NumberProperty property = (NumberProperty)device.GetProperty("WHEEL_SLOT");
          if (property != null) {
            property.SetSingleValue("SLOT", value + 1);
          }
        }
      }
    }

    override protected void propertyChanged(Property property) {
      base.propertyChanged(property);
      if (property.DeviceName == deviceName) {
        if (property.Name == "WHEEL_SLOT") {
          NumberItem slot = (NumberItem)property.GetItem("SLOT");
          if (slot != null) {
            if (property.State == Property.States.Busy) {
              position = -1;
              Console.WriteLine("Wheel is moving");
            } else {
              int count = (int)slot.Max;
              if (names.Length != count) {
                Console.WriteLine("Wheel position count " + count);
                names = new string[count];
                offsets = new int[count];
                for (int i = 0; i < count; i++) {
                  names[i] = "NAME #" + i;
                  offsets[i] = 0;
                }
              }
              position = (short)(slot.Value - 1);
              Console.WriteLine("Wheel position is " + position);
            }
          }
        } else if (property.Name == "WHEEL_SLOT_NAME") {
          if (property.State == Property.States.Idle || property.State == Property.States.Ok) {
            for (int i = 0; i < names.Length; i++) {
              TextItem slotName = (TextItem)property.GetItem("SLOT_NAME_" + (i + 1));
              if (slotName != null) {
                names[i] = slotName.Value;
                Console.WriteLine("Wheel slot name " + i + ": " + slotName.Value);
              }
            }
          }
        }
      }
    }

    private static void RegUnregASCOM(bool bRegister) {
      using (var P = new ASCOM.Utilities.Profile()) {
        P.DeviceType = "FilterWheel";
        if (bRegister) {
          P.Register(driverID, driverName);
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

    override protected void ReadProfile() {
      using (Profile driverProfile = new Profile()) {
        driverProfile.DeviceType = "FilterWheel";
        deviceName = driverProfile.GetValue(driverID, deviceNameProfileName, string.Empty, string.Empty);
      }
    }

    override protected void WriteProfile() {
      using (Profile driverProfile = new Profile()) {
        driverProfile.DeviceType = "FilterWheel";
        driverProfile.WriteValue(driverID, deviceNameProfileName, deviceName);
      }
    }
  }
}

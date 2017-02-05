//
// ASCOM CCD Driver for INDIGO
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

#define Camera

using System;
using System.Runtime.InteropServices;
using ASCOM.Astrometry.AstroUtils;
using ASCOM.Utilities;
using ASCOM.DeviceInterface;
using System.Globalization;
using System.Collections;
using INDIGO;

namespace ASCOM.INDIGO {

  [Guid("17b1bb89-ff1f-4381-8414-0205d33f9654")]
  [ClassInterface(ClassInterfaceType.None)]
  public class Camera : BaseDriver, ICameraV2 {
    internal static string driverID = "ASCOM.INDIGO.Camera";
    private static string driverName = "INDIGO Camera";

    public Camera() {
      deviceInterface = Device.InterfaceMask.CCD;
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

    private int ccdWidth, ccdHeight;
    private double pixelWidth, pixelHeight;
    private int frameLeft, frameTop, frameWidth, frameHeight;
    private short maxHorizontalBin, maxVerticalBin, horizontalBin, verticalBin;
    private int bitsPerPixel;
    private double minExposure, maxExposure;
    private bool canAbort = false, hasCooler = false, hasGain = false, hasCoolerPower = false, canSetTemperature = false;
    private bool coolerOn;
    private short minGain, maxGain, gain;
    private double temperature, targetTemperature, coolerPower;
    private CameraStates cameraState;

    private DateTime exposureStart = DateTime.MinValue;
    private double cameraLastExposureDuration = 0.0;
    private bool cameraImageReady = false;
    private int[,] cameraImageArray;
    private object[,] cameraImageArrayVariant;

    override protected void propertyAdded(Property property) {
      if (property.DeviceName == deviceName) {
        if (property.Name == "CCD_ABORT_EXPOSURE") {
          canAbort = true;
        } else if (property.Name == "CCD_EXPOSURE") {
          NumberItem item = ((NumberItem)property.GetItem("EXPOSURE"));
          minExposure = item.Min;
          maxExposure = item.Max;
        } else if (property.Name == "CCD_GAIN") {
          NumberItem item = ((NumberItem)property.GetItem("GAIN"));
          hasGain = true;
          minGain = (short)item.Min;
          maxGain = (short)item.Max;
          gain = (short)item.Value;
        } else if (property.Name == "CCD_COOLER") {
          hasCooler = true;
          coolerOn = ((SwitchItem)property.GetItem("ON")).Value;
        } else if (property.Name == "CCD_TEMPERATURE") {
          canSetTemperature = property.Permission != Property.Permissions.ReadOnly;
          NumberItem item = ((NumberItem)property.GetItem("TEMPERATURE"));
          temperature = item.Value;
          targetTemperature = item.Target;
        } else if (property.Name == "CCD_COOLER_POWER") {
          hasCoolerPower = true;
          coolerPower = ((NumberItem)property.GetItem("POWER")).Value;
        } else {
          base.propertyAdded(property);
        }
      }
    }

    override protected void propertyChanged(Property property) {
      base.propertyChanged(property);
      if (property.DeviceName == deviceName) {
        if (property.Name == "CCD_INFO") {
          foreach (Item item in property.Items) {
            NumberItem numberItem = (NumberItem)item;
            switch (numberItem.Name) {
              case "WIDTH":
                ccdWidth = (int)numberItem.Value;
                break;
              case "HEIGHT":
                ccdHeight = (int)numberItem.Value;
                break;
              case "MAX_HORIZONTAL_BIN":
                maxHorizontalBin = (short)numberItem.Value;
                break;
              case "MAX_VERTICAL_BIN":
                maxVerticalBin = (short)numberItem.Value;
                break;
              case "PIXEL_WIDTH":
                pixelWidth = numberItem.Value;
                break;
              case "PIXEL_HEIGHT":
                pixelHeight = numberItem.Value;
                break;
              case "BITS_PER_PIXEL":
                bitsPerPixel = (int)numberItem.Value;
                break;
            }
          }
        } else if (property.Name == "CCD_BIN") {
          foreach (Item item in property.Items) {
            NumberItem numberItem = (NumberItem)item;
            switch (numberItem.Name) {
              case "HORIZONTAL":
                horizontalBin = (short)numberItem.Value;
                break;
              case "VERTICAL":
                verticalBin = (short)numberItem.Value;
                break;
            }
          }
        } else if (property.Name == "CCD_FRAME") {
          foreach (Item item in property.Items) {
            NumberItem numberItem = (NumberItem)item;
            switch (numberItem.Name) {
              case "LEFT":
                frameLeft = (int)numberItem.Value;
                break;
              case "TOP":
                frameTop = (int)numberItem.Value;
                break;
              case "WIDTH":
                frameWidth = (int)numberItem.Value;
                break;
              case "HEIGHT":
                frameHeight = (int)numberItem.Value;
                break;
            }
          }
        } else if (property.Name == "CCD_GAIN") {
          gain = (short)((NumberItem)property.GetItem("GAIN")).Value;
        } else if (property.Name == "CCD_COOLER") {
          coolerOn = ((SwitchItem)property.GetItem("ON")).Value;
        } else if (property.Name == "CCD_COOLER_POWER") {
          coolerPower = ((NumberItem)property.GetItem("POWER")).Value;
        } else if (property.Name == "CCD_TEMPERATURE") {
          NumberItem item = ((NumberItem)property.GetItem("TEMPERATURE"));
          temperature = item.Value;
          targetTemperature = item.Target;
        } else if (property.Name == "CCD_EXPOSURE") {
          if (property.State == Property.States.Busy)
            cameraState = CameraStates.cameraExposing;
          else if (property.State == Property.States.Alert)
            cameraState = CameraStates.cameraError;
          else
            cameraState = CameraStates.cameraIdle;
          NumberItem item = ((NumberItem)property.GetItem("EXPOSURE"));
        }
      }
    }

    public short BayerOffsetX {
      get {
        throw new ASCOM.PropertyNotImplementedException("BayerOffsetX", false);
      }
    }

    public short BayerOffsetY {
      get {
        throw new ASCOM.PropertyNotImplementedException("BayerOffsetY", true);
      }
    }

    public short BinX {
      get {
        return horizontalBin;
      }
      set {
        if (value < 1 || value > maxHorizontalBin)
          throw new ASCOM.InvalidValueException("BinX", value.ToString(), "1.." + maxHorizontalBin);
        horizontalBin = value;
      }
    }

    public short BinY {
      get {
        return verticalBin;
      }
      set {
        if (value < 1 || value > maxVerticalBin)
          throw new ASCOM.InvalidValueException("BinY", value.ToString(), "1.." + maxVerticalBin);
        verticalBin = value;
      }
    }

    public short MaxBinX {
      get {
        return maxHorizontalBin;
      }
    }

    public short MaxBinY {
      get {
        return maxVerticalBin;
      }
    }

    public int StartX {
      get {
        return frameLeft / horizontalBin;
      }
      set {
        frameLeft = value * horizontalBin;
      }
    }

    public int StartY {
      get {
        return frameTop / verticalBin;
      }
      set {
        frameTop = value * verticalBin;
      }
    }

    public int NumX {
      get {
        return frameWidth / horizontalBin;
      }
      set {
        frameWidth = value * horizontalBin;
      }
    }

    public int NumY {
      get {
        return frameHeight / verticalBin;
      }
      set {
        frameHeight = value * verticalBin;
      }
    }

    public double PixelSizeX {
      get {
        return pixelWidth;
      }
    }

    public double PixelSizeY {
      get {
        return pixelHeight;
      }
    }

    public int CameraXSize {
      get {
        return ccdWidth;
      }
    }

    public int CameraYSize {
      get {
        return ccdHeight;
      }
    }


    public double CCDTemperature {
      get {
       return temperature;
      }
    }

    public bool CanAbortExposure {
      get {
        return canAbort;
      }
    }

    public bool CanAsymmetricBin {
      get {
        return true;
      }
    }

    public CameraStates CameraState {
      get {
        return cameraState;
      }
    }

    public bool CanFastReadout {
      get {
        return false;
      }
    }

    public bool CanGetCoolerPower {
      get {
        return hasCoolerPower;
      }
    }

    public bool CanPulseGuide {
      get {
        return false;
      }
    }

    public bool CanSetCCDTemperature {
      get {
        return canSetTemperature;
      }
    }

    public bool CanStopExposure {
      get {
        return false;
      }
    }

    public bool CoolerOn {
      get {
        if (hasCooler)
          return coolerOn;
        throw new ASCOM.PropertyNotImplementedException("CoolerOn", false);
      }
      set {
        if (hasCooler) {
          SwitchProperty property = (SwitchProperty)device.GetProperty("CCD_COOLER");
          if (value)
            property.SetSingleValue("ON", true);
          else
            property.SetSingleValue("OFF", true);
          return;
        }
        throw new ASCOM.PropertyNotImplementedException("CoolerOn", true);
      }
    }

    public double CoolerPower {
      get {
        if (hasCoolerPower)
          return coolerPower;
        throw new ASCOM.PropertyNotImplementedException("CoolerPower", false);
      }
    }

    public double ElectronsPerADU {
      get {
        throw new ASCOM.PropertyNotImplementedException("ElectronsPerADU", false);
      }
    }

    public double ExposureMax {
      get {
        return maxExposure;
      }
    }

    public double ExposureMin {
      get {
        return minExposure;
      }
    }

    public double ExposureResolution {
      get {
        return 0.0;
      }
    }

    public bool FastReadout {
      get {
        throw new ASCOM.PropertyNotImplementedException("FastReadout", false);
      }
      set {
        throw new ASCOM.PropertyNotImplementedException("FastReadout", true);
      }
    }

    public double FullWellCapacity {
      get {
        throw new ASCOM.PropertyNotImplementedException("FullWellCapacity", false);
      }
    }

    public short Gain {
      get {
        return gain;
      }
      set {
        gain = value;
      }
    }

    public short GainMax {
      get {
        return maxGain;
      }
    }

    public short GainMin {
      get {
        return minGain;
      }
    }

    public ArrayList Gains {
      get {
        throw new ASCOM.PropertyNotImplementedException("Gains", true);
      }
    }

    public bool HasShutter {
      get {
        return false;
      }
    }

    public double HeatSinkTemperature {
      get {
        throw new ASCOM.PropertyNotImplementedException("HeatSinkTemperature", false);
      }
    }

    public object ImageArray {
      get {
        if (!cameraImageReady) {
          throw new ASCOM.InvalidOperationException("Call to ImageArray before the first image has been taken!");
        }

        cameraImageArray = new int[frameWidth, frameHeight];
        return cameraImageArray;
      }
    }

    public object ImageArrayVariant {
      get {
        if (!cameraImageReady) {
          throw new ASCOM.InvalidOperationException("Call to ImageArrayVariant before the first image has been taken!");
        }
        cameraImageArrayVariant = new object[frameWidth, frameHeight];
        for (int i = 0; i < cameraImageArray.GetLength(1); i++) {
          for (int j = 0; j < cameraImageArray.GetLength(0); j++) {
            cameraImageArrayVariant[j, i] = cameraImageArray[j, i];
          }

        }

        return cameraImageArrayVariant;
      }
    }

    public bool ImageReady {
      get {
        return cameraImageReady;
      }
    }

    public bool IsPulseGuiding {
      get {
        throw new ASCOM.PropertyNotImplementedException("IsPulseGuiding", false);
      }
    }

    public double LastExposureDuration {
      get {
        if (!cameraImageReady) {
          throw new ASCOM.InvalidOperationException("Call to LastExposureDuration before the first image has been taken!");
        }
        return cameraLastExposureDuration;
      }
    }

    public string LastExposureStartTime {
      get {
        if (!cameraImageReady) {
          throw new ASCOM.InvalidOperationException("Call to LastExposureStartTime before the first image has been taken!");
        }
        string exposureStartString = exposureStart.ToString("yyyy-MM-ddTHH:mm:ss");
        return exposureStartString;
      }
    }

    public int MaxADU {
      get {
        return 20000;
      }
    }

    public short PercentCompleted {
      get {
        throw new ASCOM.PropertyNotImplementedException("PercentCompleted", false);
      }
    }

    public void PulseGuide(GuideDirections Direction, int Duration) {
      throw new ASCOM.MethodNotImplementedException("PulseGuide");
    }

    public short ReadoutMode {
      get {
        throw new ASCOM.PropertyNotImplementedException("ReadoutMode", false);
      }
      set {
        throw new ASCOM.PropertyNotImplementedException("ReadoutMode", true);
      }
    }

    public ArrayList ReadoutModes {
      get {
        throw new ASCOM.PropertyNotImplementedException("ReadoutModes", false);
      }
    }

    public string SensorName {
      get {
        throw new ASCOM.PropertyNotImplementedException("SensorName", false);
      }
    }

    public SensorType SensorType {
      get {
        throw new ASCOM.PropertyNotImplementedException("SensorType", false);
      }
    }

    public double SetCCDTemperature {
      get {
        return targetTemperature;
      }
      set {
        NumberProperty property = (NumberProperty)device.GetProperty("CCD_TEMPERATURE");
        if (property != null) {
          property.SetSingleValue("TEMPERATURE", value);
        }
      }
    }

    public void StartExposure(double Duration, bool Light) {
      if (Duration < 0.0)
        throw new InvalidValueException("StartExposure", Duration.ToString(), "0.0 upwards");
      if (frameWidth > ccdWidth)
        throw new InvalidValueException("StartExposure", frameWidth.ToString(), ccdWidth.ToString());
      if (frameHeight > ccdHeight)
        throw new InvalidValueException("StartExposure", frameHeight.ToString(), ccdHeight.ToString());
      if (frameLeft > ccdWidth)
        throw new InvalidValueException("StartExposure", frameLeft.ToString(), ccdWidth.ToString());
      if (frameTop > ccdHeight)
        throw new InvalidValueException("StartExposure", frameTop.ToString(), ccdHeight.ToString());

      cameraLastExposureDuration = Duration;
      exposureStart = DateTime.Now;
      System.Threading.Thread.Sleep((int)Duration * 1000);  // Sleep for the duration to simulate exposure 
      cameraImageReady = true;
    }

    public void AbortExposure() {
      SwitchProperty property = (SwitchProperty)device.GetProperty("CCD_ABORT_EXPOSURE");
      if (property != null) {
        property.SetSingleValue("ABORT_EXPOSURE", true);
      }
    }

    public void StopExposure() {
      throw new MethodNotImplementedException("StopExposure");
    }

    private static void RegUnregASCOM(bool bRegister) {
      using (var P = new ASCOM.Utilities.Profile()) {
        P.DeviceType = "Camera";
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
        driverProfile.DeviceType = "Camera";
        deviceName = driverProfile.GetValue(driverID, deviceNameProfileName, string.Empty, string.Empty);
      }
    }

    override protected void WriteProfile() {
      using (Profile driverProfile = new Profile()) {
        driverProfile.DeviceType = "Camera";
        driverProfile.WriteValue(driverID, deviceNameProfileName, deviceName);
      }
    }
  }
}

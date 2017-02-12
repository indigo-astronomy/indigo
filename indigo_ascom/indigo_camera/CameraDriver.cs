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
using ASCOM.Utilities;
using ASCOM.DeviceInterface;
using System.Globalization;
using System.Collections;
using INDIGO;
using System.Net;
using System.IO;
using System.Threading;

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
    private short maxHorizontalBin, maxVerticalBin, horizontalBin = 1, verticalBin = 1;
    private int bitsPerPixel;
    private double minExposure, maxExposure;
    private bool canAbort = false, hasCooler = false, hasGain = false, hasCoolerPower = false, canSetTemperature = false;
    private bool coolerOn;
    private short minGain, maxGain, gain;
    private double temperature, targetTemperature, coolerPower;
    private CameraStates cameraState;
    private short percentCompleted;
    private bool firstExposure = true;
    private DateTime exposureStart = DateTime.MinValue;
    private double exposureDuration = 0.0;
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

    override protected void propertyUpdated(Property property) {
      base.propertyUpdated(property);
      if (property.DeviceName == deviceName) {
        if (property.Name == "CCD_IMAGE" && property.State == Property.States.Ok) {
          BLOBItem blobItem = ((BLOBItem)property.GetItem("IMAGE"));
          if (blobItem != null) {
            Server server = property.Device.Server;
            String url = "http://" + server.Host + ":" + server.Port + blobItem.Value;
            Log("Request" + url);
            WebRequest request = WebRequest.Create(url);
            WebResponse response = request.GetResponse();
            Log("Response ContentLength " + response.ContentLength);
            BinaryReader reader = new BinaryReader(response.GetResponseStream());
            cameraImageArray = new int[frameWidth, frameHeight];
            for (int y = 0; y < frameHeight; y++)
              for (int x = 0; x < frameWidth; x++)
                cameraImageArray[x, y] = reader.ReadUInt16();
            reader.Close();
            percentCompleted = 100;
            cameraImageReady = true;
            cameraState = CameraStates.cameraIdle;
          }
        } else if (property.Name == "CCD_EXPOSURE") {
          NumberItem item = ((NumberItem)property.GetItem("EXPOSURE"));
          if (property.State == Property.States.Ok) {
          } else if (property.State == Property.States.Busy) {
            cameraState = CameraStates.cameraExposing;
            percentCompleted = (short)(100 - 100 * item.Value / item.Target);
          } else {
            cameraState = CameraStates.cameraError;
            percentCompleted = 0;
          }
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
                frameLeft = 0;
                frameWidth = ccdWidth;
                break;
              case "HEIGHT":
                ccdHeight = (int)numberItem.Value;
                frameTop = 0;
                frameHeight = ccdHeight;
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
        for (int i = 1; i <= maxHorizontalBin; i = i << 1) {
          if (i == value) {
            horizontalBin = value;
            return;
          }
        }
        throw new ASCOM.InvalidValueException("BinX", value.ToString(), "1.." + maxHorizontalBin);
      }
    }

    public short BinY {
      get {
        return verticalBin;
      }
      set {
        for (int i = 1; i <= maxVerticalBin; i = i << 1) {
          if (i == value) {
            verticalBin = value;
            return;
          }
        }
        throw new ASCOM.InvalidValueException("BinY", value.ToString(), "1.." + maxVerticalBin);
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
        return frameLeft;
      }
      set {
        frameLeft = value;
      }
    }

    public int StartY {
      get {
        return frameTop;
      }
      set {
        frameTop = value;
      }
    }

    public int NumX {
      get {
        return frameWidth;
      }
      set {
        frameWidth = value;
      }
    }

    public int NumY {
      get {
        return frameHeight;
      }
      set {
        frameHeight = value;
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
        return cameraImageArray;
      }
    }

    public object ImageArrayVariant {
      get {
        if (!cameraImageReady) {
          throw new ASCOM.InvalidOperationException("Call to ImageArrayVariant before the first image has been taken!");
        }
        cameraImageArrayVariant = new object[frameWidth, frameHeight];
        for (int j = 0; j < frameWidth; j++) {
          for (int i = 0; i < frameHeight; i++) {
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
        return false;
      }
    }

    public double LastExposureDuration {
      get {
        if (firstExposure) {
          throw new ASCOM.InvalidOperationException("Call to LastExposureDuration before the first image has been taken!");
        }
        return exposureDuration;
      }
    }

    public string LastExposureStartTime {
      get {
        if (firstExposure) {
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
        return percentCompleted;
      }
    }

    public void PulseGuide(GuideDirections Direction, int Duration) {
      throw new ASCOM.MethodNotImplementedException("PulseGuide");
    }

    public short ReadoutMode {
      get {
        return 0;
      }
      set {
      }
    }

    public ArrayList ReadoutModes {
      get {
        return new ArrayList() { "Normal" };
      }
    }

    public string SensorName {
      get {
        return "UNKNOWN";
      }
    }

    public SensorType SensorType {
      get {
        return SensorType.Monochrome;
      }
    }

    public double SetCCDTemperature {
      get {
        return targetTemperature;
      }
      set {
        if (CanSetCCDTemperature) {
          NumberProperty property = (NumberProperty)device.GetProperty("CCD_TEMPERATURE");
          if (property != null) {
            property.SetSingleValue("TEMPERATURE", value);
          }
        } else {
          throw new PropertyNotImplementedException("SetCCDTemperature", true);
        }
      }
    }

    public void StartExposure(double Duration, bool Light) {
      if (Duration < 0.0)
        throw new InvalidValueException("StartExposure", Duration.ToString(), "0.0 upwards");
      if (frameWidth * horizontalBin > ccdWidth)
        throw new InvalidValueException("StartExposure " + frameWidth + "*" + horizontalBin, frameWidth.ToString(), ccdWidth.ToString());
      if (frameHeight * verticalBin > ccdHeight)
        throw new InvalidValueException("StartExposure " + frameHeight + "*" + verticalBin, frameHeight.ToString(), ccdHeight.ToString());
      if (frameLeft * horizontalBin > ccdWidth)
        throw new InvalidValueException("StartExposure " + frameLeft + "*" + horizontalBin, frameLeft.ToString(), ccdWidth.ToString());
      if (frameTop * verticalBin > ccdHeight)
        throw new InvalidValueException("StartExposure " + frameHeight + "*" + verticalBin, frameTop.ToString(), ccdHeight.ToString());
      exposureDuration = Duration;
      exposureStart = DateTime.Now;
      cameraImageReady = false;
      firstExposure = false;
      NumberProperty n = (NumberProperty)device.GetProperty("CCD_BIN");
      if (n != null) {
        ((NumberItem)n.GetItem("HORIZONTAL")).Value = horizontalBin;
        ((NumberItem)n.GetItem("VERTICAL")).Value = verticalBin;
        n.Change();
      }
      n = (NumberProperty)device.GetProperty("CCD_FRAME");
      if (n != null) {
        ((NumberItem)n.GetItem("LEFT")).Value = frameLeft * horizontalBin;
        ((NumberItem)n.GetItem("TOP")).Value = frameTop * verticalBin;
        ((NumberItem)n.GetItem("WIDTH")).Value = frameWidth * horizontalBin;
        ((NumberItem)n.GetItem("HEIGHT")).Value = frameHeight * verticalBin;
        n.Change();
      }
      if (hasGain) {
        n = (NumberProperty)device.GetProperty("CCD_GAIN");
        n.SetSingleValue("GAIN", gain);
      }
      SwitchProperty s = (SwitchProperty)device.GetProperty("CCD_FRAME_TYPE");
      if (s != null) {
        if (Light)
          s.SetSingleValue("LIGHT", true);
        else
          s.SetSingleValue("DARK", true);
      }
      s = (SwitchProperty)device.GetProperty("CCD_IMAGE_FORMAT");
      if (s != null)
        s.SetSingleValue("RAW", true);
      n = (NumberProperty)device.GetProperty("CCD_EXPOSURE");
      cameraState = CameraStates.cameraExposing;
      n.SetSingleValue("EXPOSURE", Duration);
      while (cameraState == CameraStates.cameraExposing) {
        Thread.Sleep(100);
      }
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

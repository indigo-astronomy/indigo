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

    private const int ccdWidth = 1394; // Constants to define the ccd pixel dimenstions
    private const int ccdHeight = 1040;
    private const double pixelSize = 6.45; // Constant for the pixel physical dimension

    private int cameraNumX = ccdWidth; // Initialise variables to hold values required for functionality tested by Conform
    private int cameraNumY = ccdHeight;
    private int cameraStartX = 0;
    private int cameraStartY = 0;
    private DateTime exposureStart = DateTime.MinValue;
    private double cameraLastExposureDuration = 0.0;
    private bool cameraImageReady = false;
    private int[,] cameraImageArray;
    private object[,] cameraImageArrayVariant;

    public void AbortExposure() {
      throw new MethodNotImplementedException("AbortExposure");
    }

    public short BayerOffsetX {
      get {
        throw new ASCOM.PropertyNotImplementedException("BayerOffsetX", false);
      }
    }

    public short BayerOffsetY {
      get {
        throw new ASCOM.PropertyNotImplementedException("BayerOffsetX", true);
      }
    }

    public short BinX {
      get {
        return 1;
      }
      set {
        if (value != 1)
          throw new ASCOM.InvalidValueException("BinX", value.ToString(), "1"); // Only 1 is valid in this simple template
      }
    }

    public short BinY {
      get {
        return 1;
      }
      set {
        if (value != 1)
          throw new ASCOM.InvalidValueException("BinY", value.ToString(), "1"); // Only 1 is valid in this simple template
      }
    }

    public double CCDTemperature {
      get {
        throw new ASCOM.PropertyNotImplementedException("CCDTemperature", false);
      }
    }

    public CameraStates CameraState {
      get {
        return CameraStates.cameraIdle;
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

    public bool CanAbortExposure {
      get {
        return false;
      }
    }

    public bool CanAsymmetricBin {
      get {
        return false;
      }
    }

    public bool CanFastReadout {
      get {
        return false;
      }
    }

    public bool CanGetCoolerPower {
      get {
        return false;
      }
    }

    public bool CanPulseGuide {
      get {
        return false;
      }
    }

    public bool CanSetCCDTemperature {
      get {
        return false;
      }
    }

    public bool CanStopExposure {
      get {
        return false;
      }
    }

    public bool CoolerOn {
      get {
        throw new ASCOM.PropertyNotImplementedException("CoolerOn", false);
      }
      set {
        throw new ASCOM.PropertyNotImplementedException("CoolerOn", true);
      }
    }

    public double CoolerPower {
      get {
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
        throw new ASCOM.PropertyNotImplementedException("ExposureMax", false);
      }
    }

    public double ExposureMin {
      get {
        throw new ASCOM.PropertyNotImplementedException("ExposureMin", false);
      }
    }

    public double ExposureResolution {
      get {
        throw new ASCOM.PropertyNotImplementedException("ExposureResolution", false);
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
        throw new ASCOM.PropertyNotImplementedException("Gain", false);
      }
      set {
        throw new ASCOM.PropertyNotImplementedException("Gain", true);
      }
    }

    public short GainMax {
      get {
        throw new ASCOM.PropertyNotImplementedException("GainMax", false);
      }
    }

    public short GainMin {
      get {
        throw new ASCOM.PropertyNotImplementedException("GainMin", true);
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

        cameraImageArray = new int[cameraNumX, cameraNumY];
        return cameraImageArray;
      }
    }

    public object ImageArrayVariant {
      get {
        if (!cameraImageReady) {
          throw new ASCOM.InvalidOperationException("Call to ImageArrayVariant before the first image has been taken!");
        }
        cameraImageArrayVariant = new object[cameraNumX, cameraNumY];
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

    public short MaxBinX {
      get {
        return 1;
      }
    }

    public short MaxBinY {
      get {
        return 1;
      }
    }

    public int NumX {
      get {
        return cameraNumX;
      }
      set {
        cameraNumX = value;
      }
    }

    public int NumY {
      get {
        return cameraNumY;
      }
      set {
        cameraNumY = value;
      }
    }

    public short PercentCompleted {
      get {
        throw new ASCOM.PropertyNotImplementedException("PercentCompleted", false);
      }
    }

    public double PixelSizeX {
      get {
        return pixelSize;
      }
    }

    public double PixelSizeY {
      get {
        return pixelSize;
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
        throw new ASCOM.PropertyNotImplementedException("SetCCDTemperature", false);
      }
      set {
        throw new ASCOM.PropertyNotImplementedException("SetCCDTemperature", true);
      }
    }

    public void StartExposure(double Duration, bool Light) {
      if (Duration < 0.0)
        throw new InvalidValueException("StartExposure", Duration.ToString(), "0.0 upwards");
      if (cameraNumX > ccdWidth)
        throw new InvalidValueException("StartExposure", cameraNumX.ToString(), ccdWidth.ToString());
      if (cameraNumY > ccdHeight)
        throw new InvalidValueException("StartExposure", cameraNumY.ToString(), ccdHeight.ToString());
      if (cameraStartX > ccdWidth)
        throw new InvalidValueException("StartExposure", cameraStartX.ToString(), ccdWidth.ToString());
      if (cameraStartY > ccdHeight)
        throw new InvalidValueException("StartExposure", cameraStartY.ToString(), ccdHeight.ToString());

      cameraLastExposureDuration = Duration;
      exposureStart = DateTime.Now;
      System.Threading.Thread.Sleep((int)Duration * 1000);  // Sleep for the duration to simulate exposure 
      cameraImageReady = true;
    }

    public int StartX {
      get {
        return cameraStartX;
      }
      set {
        cameraStartX = value;
      }
    }

    public int StartY {
      get {
        return cameraStartY;
      }
      set {
        cameraStartY = value;
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

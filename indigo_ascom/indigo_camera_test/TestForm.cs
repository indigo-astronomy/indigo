using System;
using System.Threading;
using System.Timers;
using System.Windows.Forms;

namespace ASCOM.INDIGO {
  public partial class TestForm : Form {

    private DriverAccess.Camera driver;
    private System.Timers.Timer timer;

    public TestForm() {
      InitializeComponent();
      SetUIState();
      timer = new System.Timers.Timer();
      timer.Elapsed += new ElapsedEventHandler(timerEvent);
      timer.Interval = 100;
      timer.Enabled = false;
    }

    private void timerEvent(object source, ElapsedEventArgs e) {
      BeginInvoke(new MethodInvoker(() => {
        labelState.Text = driver.CameraState.ToString();
        switch (driver.CameraState) {
          case DeviceInterface.CameraStates.cameraError:
          case DeviceInterface.CameraStates.cameraIdle:
            timer.Enabled = false;
            break;
          case DeviceInterface.CameraStates.cameraExposing:
          case DeviceInterface.CameraStates.cameraReading:
          case DeviceInterface.CameraStates.cameraDownload:
          case DeviceInterface.CameraStates.cameraWaiting:
            progressBarComplete.Value = driver.PercentCompleted;
            break;
        }
      }));
    }

    private void TestForm_FormClosing(object sender, FormClosingEventArgs e) {
      if (IsConnected)
        driver.Connected = false;
      Properties.Settings.Default.Save();
    }

    private void buttonChoose_Click(object sender, EventArgs e) {
      Properties.Settings.Default.DriverId = ASCOM.DriverAccess.Camera.Choose(Properties.Settings.Default.DriverId);
      SetUIState();
    }

    private void buttonConnect_Click(object sender, EventArgs e) {
      if (IsConnected) {
        driver.Connected = false;
          driver.Dispose();
        driver = null;
      } else {
        driver = new ASCOM.DriverAccess.Camera(Properties.Settings.Default.DriverId);
        driver.Connected = true;
      }
      SetUIState();
    }

    private void buttonStart_Click(object sender, EventArgs e) {
      buttonStart.Enabled = false;
      buttonAbort.Enabled = true;
      timer.Enabled = true;
      ThreadPool.QueueUserWorkItem(delegate {
        driver.StartX = Convert.ToInt16(textBoxLeft.Text);
        driver.StartY = Convert.ToInt16(textBoxTop.Text);
        driver.NumX = Convert.ToInt16(textBoxWidth.Text);
        driver.NumY = Convert.ToInt16(textBoxHeight.Text);
        driver.BinX = Convert.ToInt16(textBoxHorizontalBin.Text);
        driver.BinY = Convert.ToInt16(textBoxVerticalBin.Text);
        driver.StartExposure(Convert.ToDouble(textBoxDuration.Text), checkBoxLight.Checked);
        BeginInvoke(new MethodInvoker(() => {
          if (driver.ImageReady) {
            progressBarComplete.Value = 100;
          } else {
            progressBarComplete.Value = 0;
          }
          buttonStart.Enabled = true;
          buttonAbort.Enabled = false;
        }));
      });
    }

    private void buttonAbort_Click(object sender, EventArgs e) {
      driver.AbortExposure();
    }

    private void SetUIState() {
      buttonConnect.Enabled = !string.IsNullOrEmpty(Properties.Settings.Default.DriverId);
      if (IsConnected) {
        buttonChoose.Enabled = false;
        labelDescription.Text = driver.Description;
        labelCCD.Text = driver.CameraXSize + " x " + driver.CameraYSize + " pixels, max binning " + driver.MaxBinX + " x " + driver.MaxBinY + ", pixel size " + driver.PixelSizeX +" x " + driver.PixelSizeY;
        textBoxLeft.Text = driver.StartX.ToString();
        textBoxTop.Text = driver.StartY.ToString();
        textBoxWidth.Text = driver.NumX.ToString();
        textBoxHeight.Text = driver.NumY.ToString();
        textBoxHorizontalBin.Text = driver.BinX.ToString();
        textBoxVerticalBin.Text = driver.BinY.ToString();
        try {
          textBoxDuration.Text = driver.LastExposureDuration.ToString();
        } catch {
          textBoxDuration.Text = "10";
        }
        checkBoxLight.Checked = true;
        progressBarComplete.Value = 0;
        buttonConnect.Text = "Disconnect";
        buttonStart.Enabled = true;
        buttonAbort.Enabled = false;
      } else {
        buttonChoose.Enabled = true;
        labelDescription.Text = string.Empty;
        labelCCD.Text = string.Empty;
        textBoxLeft.Text = string.Empty;
        textBoxTop.Text = string.Empty;
        textBoxWidth.Text = string.Empty;
        textBoxHeight.Text = string.Empty;
        textBoxHorizontalBin.Text = string.Empty;
        textBoxVerticalBin.Text = string.Empty;
        textBoxDuration.Text = string.Empty;
        checkBoxLight.Checked = false;
        progressBarComplete.Value = 0;
        buttonConnect.Text = "Connect";
        buttonStart.Enabled = false;
        buttonAbort.Enabled = false;
      }
    }

    private bool IsConnected {
      get {
        return ((this.driver != null) && (driver.Connected == true));
      }
    }
  }
}

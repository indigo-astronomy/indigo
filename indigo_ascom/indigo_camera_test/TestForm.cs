using System;
using System.Windows.Forms;

namespace ASCOM.INDIGO {
  public partial class TestForm : Form {

    private ASCOM.DriverAccess.Camera driver;

    public TestForm() {
      InitializeComponent();
      SetUIState();
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
      } else {
        driver = new ASCOM.DriverAccess.Camera(Properties.Settings.Default.DriverId);
        driver.Connected = true;
      }
      SetUIState();
    }

    private void SetUIState() {
      buttonConnect.Enabled = !string.IsNullOrEmpty(Properties.Settings.Default.DriverId);
      if (IsConnected) {
        buttonChoose.Enabled = false;
        buttonConnect.Text = "Disconnect";
      } else {
        buttonChoose.Enabled = true;
        buttonConnect.Text = "Connect";
      }
    }

    private bool IsConnected {
      get {
        return ((this.driver != null) && (driver.Connected == true));
      }
    }
  }
}

using System;
using System.Windows.Forms;

namespace ASCOM.INDIGO {
  public partial class TestForm : Form {

    private ASCOM.DriverAccess.FilterWheel driver;

    public TestForm() {
      InitializeComponent();
      SetUIState();
    }

    private void Form1_FormClosing(object sender, FormClosingEventArgs e) {
      if (IsConnected)
        driver.Connected = false;

      Properties.Settings.Default.Save();
    }

    private void buttonChoose_Click(object sender, EventArgs e) {
      Properties.Settings.Default.DriverId = ASCOM.DriverAccess.FilterWheel.Choose(Properties.Settings.Default.DriverId);
      SetUIState();
    }

    private void buttonConnect_Click(object sender, EventArgs e) {
      if (IsConnected) {
        driver.Connected = false;
      } else {
        if (driver == null)
          driver = new ASCOM.DriverAccess.FilterWheel(Properties.Settings.Default.DriverId);
        driver.Connected = true;
      }
      SetUIState();
    }

    private void buttonGo_Click(object sender, EventArgs e) {
      driver.Position = (short)listBoxSlots.SelectedIndex;
    }

    private void SetUIState() {
      buttonConnect.Enabled = !string.IsNullOrEmpty(Properties.Settings.Default.DriverId);
      if (IsConnected) {
        buttonChoose.Enabled = false;
        buttonGo.Enabled = true;
        buttonConnect.Text = "Disconnect";
        listBoxSlots.Items.AddRange(driver.Names);
      } else {
        buttonChoose.Enabled = true;
        buttonGo.Enabled = false;
        buttonConnect.Text = "Connect";
        listBoxSlots.Items.Clear();
      }
    }

    private bool IsConnected {
      get {
        return ((this.driver != null) && (driver.Connected == true));
      }
    }
  }
}

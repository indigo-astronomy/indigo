using INDIGO;
using System;
using System.Windows.Forms;

namespace ASCOM.INDIGO {
  public partial class WaitForForm : Form {
    public WaitForForm() {
      InitializeComponent();
      StartPosition = FormStartPosition.CenterScreen;
    }

    public DialogResult Wait(out bool key, string reason, BaseDriver driver) {
      this.reason.Text = reason;
      key = true;
      driver.Log("Wait for: " + reason);
      DialogResult result = ShowDialog();
      key = false;
      return result;
    }

    public void Hide(ref bool key) {
      if (key) {
        key = false;
        BeginInvoke(new MethodInvoker(() => {
          Close();
        }));
      }
    }

    private void cancelButton_Click(object sender, EventArgs e) {
      Close();
    }
  }
}

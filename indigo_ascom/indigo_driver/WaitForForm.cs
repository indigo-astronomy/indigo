using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using INDIGO;

namespace ASCOM.INDIGO {
  public partial class WaitForForm : Form {
    public WaitForForm() {
      InitializeComponent();
      StartPosition = FormStartPosition.CenterScreen;
    }

    public DialogResult Wait(out bool key, string reason) {
      this.reason.Text = reason;
      key = true;
      Console.WriteLine(reason);
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

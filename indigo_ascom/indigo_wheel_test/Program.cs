using System;
using System.Windows.Forms;

namespace ASCOM.INDIGO {
  static class Program {
    [STAThread]
    static void Main() {
      Application.EnableVisualStyles();
      Application.SetCompatibleTextRenderingDefault(false);
      Application.Run(new TestForm());
    }
  }
}

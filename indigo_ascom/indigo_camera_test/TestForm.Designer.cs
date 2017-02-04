namespace ASCOM.INDIGO {
  partial class TestForm {
    /// <summary>
    /// Required designer variable.
    /// </summary>
    private System.ComponentModel.IContainer components = null;

    /// <summary>
    /// Clean up any resources being used.
    /// </summary>
    /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
    protected override void Dispose(bool disposing) {
      if (disposing && (components != null)) {
        components.Dispose();
      }
      base.Dispose(disposing);
    }

    #region Windows Form Designer generated code

    /// <summary>
    /// Required method for Designer support - do not modify
    /// the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent() {
      this.buttonChoose = new System.Windows.Forms.Button();
      this.buttonConnect = new System.Windows.Forms.Button();
      this.labelDriverId = new System.Windows.Forms.Label();
      this.SuspendLayout();
      // 
      // buttonChoose
      // 
      this.buttonChoose.Location = new System.Drawing.Point(309, 10);
      this.buttonChoose.Name = "buttonChoose";
      this.buttonChoose.Size = new System.Drawing.Size(72, 23);
      this.buttonChoose.TabIndex = 0;
      this.buttonChoose.Text = "Choose";
      this.buttonChoose.UseVisualStyleBackColor = true;
      this.buttonChoose.Click += new System.EventHandler(this.buttonChoose_Click);
      // 
      // buttonConnect
      // 
      this.buttonConnect.Location = new System.Drawing.Point(309, 39);
      this.buttonConnect.Name = "buttonConnect";
      this.buttonConnect.Size = new System.Drawing.Size(72, 23);
      this.buttonConnect.TabIndex = 1;
      this.buttonConnect.Text = "Connect";
      this.buttonConnect.UseVisualStyleBackColor = true;
      this.buttonConnect.Click += new System.EventHandler(this.buttonConnect_Click);
      // 
      // labelDriverId
      // 
      this.labelDriverId.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
      this.labelDriverId.DataBindings.Add(new System.Windows.Forms.Binding("Text", global::ASCOM.INDIGO.Properties.Settings.Default, "DriverId", true, System.Windows.Forms.DataSourceUpdateMode.OnPropertyChanged));
      this.labelDriverId.Location = new System.Drawing.Point(12, 40);
      this.labelDriverId.Name = "labelDriverId";
      this.labelDriverId.Size = new System.Drawing.Size(291, 21);
      this.labelDriverId.TabIndex = 2;
      this.labelDriverId.Text = global::ASCOM.INDIGO.Properties.Settings.Default.DriverId;
      this.labelDriverId.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // TestForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(409, 262);
      this.Controls.Add(this.labelDriverId);
      this.Controls.Add(this.buttonConnect);
      this.Controls.Add(this.buttonChoose);
      this.Name = "TestForm";
      this.Text = "INDIGO CCD Tester";
      this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.TestForm_FormClosing);
      this.ResumeLayout(false);

    }

    #endregion

    private System.Windows.Forms.Button buttonChoose;
    private System.Windows.Forms.Button buttonConnect;
    private System.Windows.Forms.Label labelDriverId;
  }
}


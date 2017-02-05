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
      this.labelDescription = new System.Windows.Forms.Label();
      this.label1 = new System.Windows.Forms.Label();
      this.label2 = new System.Windows.Forms.Label();
      this.labelCCD = new System.Windows.Forms.Label();
      this.labelFrame = new System.Windows.Forms.Label();
      this.label4 = new System.Windows.Forms.Label();
      this.SuspendLayout();
      // 
      // buttonChoose
      // 
      this.buttonChoose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
      this.buttonChoose.Location = new System.Drawing.Point(500, 9);
      this.buttonChoose.Name = "buttonChoose";
      this.buttonChoose.Size = new System.Drawing.Size(72, 21);
      this.buttonChoose.TabIndex = 0;
      this.buttonChoose.Text = "Choose";
      this.buttonChoose.UseVisualStyleBackColor = true;
      this.buttonChoose.Click += new System.EventHandler(this.buttonChoose_Click);
      // 
      // buttonConnect
      // 
      this.buttonConnect.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
      this.buttonConnect.Location = new System.Drawing.Point(500, 36);
      this.buttonConnect.Name = "buttonConnect";
      this.buttonConnect.Size = new System.Drawing.Size(72, 21);
      this.buttonConnect.TabIndex = 1;
      this.buttonConnect.Text = "Connect";
      this.buttonConnect.UseVisualStyleBackColor = true;
      this.buttonConnect.Click += new System.EventHandler(this.buttonConnect_Click);
      // 
      // labelDriverId
      // 
      this.labelDriverId.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.labelDriverId.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
      this.labelDriverId.DataBindings.Add(new System.Windows.Forms.Binding("Text", global::ASCOM.INDIGO.Properties.Settings.Default, "DriverId", true, System.Windows.Forms.DataSourceUpdateMode.OnPropertyChanged));
      this.labelDriverId.Location = new System.Drawing.Point(12, 9);
      this.labelDriverId.Name = "labelDriverId";
      this.labelDriverId.Size = new System.Drawing.Size(482, 21);
      this.labelDriverId.TabIndex = 2;
      this.labelDriverId.Text = global::ASCOM.INDIGO.Properties.Settings.Default.DriverId;
      this.labelDriverId.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // labelDescription
      // 
      this.labelDescription.AutoSize = true;
      this.labelDescription.Location = new System.Drawing.Point(90, 40);
      this.labelDescription.Name = "labelDescription";
      this.labelDescription.Size = new System.Drawing.Size(58, 13);
      this.labelDescription.TabIndex = 3;
      this.labelDescription.Text = "desctiption";
      // 
      // label1
      // 
      this.label1.AutoSize = true;
      this.label1.Location = new System.Drawing.Point(12, 40);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(61, 13);
      this.label1.TabIndex = 4;
      this.label1.Text = "description:";
      // 
      // label2
      // 
      this.label2.AutoSize = true;
      this.label2.Location = new System.Drawing.Point(12, 65);
      this.label2.Name = "label2";
      this.label2.Size = new System.Drawing.Size(32, 13);
      this.label2.TabIndex = 5;
      this.label2.Text = "CCD:";
      // 
      // labelCCD
      // 
      this.labelCCD.AutoSize = true;
      this.labelCCD.Location = new System.Drawing.Point(90, 65);
      this.labelCCD.Name = "labelCCD";
      this.labelCCD.Size = new System.Drawing.Size(25, 13);
      this.labelCCD.TabIndex = 6;
      this.labelCCD.Text = "ccd";
      // 
      // labelFrame
      // 
      this.labelFrame.AutoSize = true;
      this.labelFrame.Location = new System.Drawing.Point(90, 90);
      this.labelFrame.Name = "labelFrame";
      this.labelFrame.Size = new System.Drawing.Size(33, 13);
      this.labelFrame.TabIndex = 8;
      this.labelFrame.Text = "frame";
      // 
      // label4
      // 
      this.label4.AutoSize = true;
      this.label4.Location = new System.Drawing.Point(12, 90);
      this.label4.Name = "label4";
      this.label4.Size = new System.Drawing.Size(39, 13);
      this.label4.TabIndex = 7;
      this.label4.Text = "Frame:";
      // 
      // TestForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(584, 675);
      this.Controls.Add(this.labelFrame);
      this.Controls.Add(this.label4);
      this.Controls.Add(this.labelCCD);
      this.Controls.Add(this.label2);
      this.Controls.Add(this.label1);
      this.Controls.Add(this.labelDescription);
      this.Controls.Add(this.labelDriverId);
      this.Controls.Add(this.buttonConnect);
      this.Controls.Add(this.buttonChoose);
      this.Name = "TestForm";
      this.Text = "INDIGO CCD Tester";
      this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.TestForm_FormClosing);
      this.ResumeLayout(false);
      this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.Button buttonChoose;
    private System.Windows.Forms.Button buttonConnect;
    private System.Windows.Forms.Label labelDriverId;
    private System.Windows.Forms.Label labelDescription;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.Label labelCCD;
    private System.Windows.Forms.Label labelFrame;
    private System.Windows.Forms.Label label4;
  }
}


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
      this.label4 = new System.Windows.Forms.Label();
      this.textBoxLeft = new System.Windows.Forms.TextBox();
      this.textBoxTop = new System.Windows.Forms.TextBox();
      this.textBoxWidth = new System.Windows.Forms.TextBox();
      this.textBoxHeight = new System.Windows.Forms.TextBox();
      this.label3 = new System.Windows.Forms.Label();
      this.textBoxVerticalBin = new System.Windows.Forms.TextBox();
      this.textBoxHorizontalBin = new System.Windows.Forms.TextBox();
      this.checkBoxLight = new System.Windows.Forms.CheckBox();
      this.textBoxDuration = new System.Windows.Forms.TextBox();
      this.label5 = new System.Windows.Forms.Label();
      this.buttonStart = new System.Windows.Forms.Button();
      this.buttonAbort = new System.Windows.Forms.Button();
      this.progressBarComplete = new System.Windows.Forms.ProgressBar();
      this.label6 = new System.Windows.Forms.Label();
      this.labelState = new System.Windows.Forms.Label();
      this.buttonSetup = new System.Windows.Forms.Button();
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
      this.buttonConnect.Location = new System.Drawing.Point(500, 63);
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
      // label4
      // 
      this.label4.AutoSize = true;
      this.label4.Location = new System.Drawing.Point(12, 90);
      this.label4.Name = "label4";
      this.label4.Size = new System.Drawing.Size(39, 13);
      this.label4.TabIndex = 7;
      this.label4.Text = "Frame:";
      // 
      // textBoxLeft
      // 
      this.textBoxLeft.Location = new System.Drawing.Point(93, 87);
      this.textBoxLeft.Name = "textBoxLeft";
      this.textBoxLeft.Size = new System.Drawing.Size(55, 20);
      this.textBoxLeft.TabIndex = 8;
      // 
      // textBoxTop
      // 
      this.textBoxTop.Location = new System.Drawing.Point(154, 87);
      this.textBoxTop.Name = "textBoxTop";
      this.textBoxTop.Size = new System.Drawing.Size(55, 20);
      this.textBoxTop.TabIndex = 9;
      // 
      // textBoxWidth
      // 
      this.textBoxWidth.Location = new System.Drawing.Point(215, 87);
      this.textBoxWidth.Name = "textBoxWidth";
      this.textBoxWidth.Size = new System.Drawing.Size(55, 20);
      this.textBoxWidth.TabIndex = 10;
      // 
      // textBoxHeight
      // 
      this.textBoxHeight.Location = new System.Drawing.Point(276, 87);
      this.textBoxHeight.Name = "textBoxHeight";
      this.textBoxHeight.Size = new System.Drawing.Size(55, 20);
      this.textBoxHeight.TabIndex = 11;
      // 
      // label3
      // 
      this.label3.AutoSize = true;
      this.label3.Location = new System.Drawing.Point(12, 116);
      this.label3.Name = "label3";
      this.label3.Size = new System.Drawing.Size(45, 13);
      this.label3.TabIndex = 12;
      this.label3.Text = "Binning:";
      // 
      // textBoxVerticalBin
      // 
      this.textBoxVerticalBin.Location = new System.Drawing.Point(154, 113);
      this.textBoxVerticalBin.Name = "textBoxVerticalBin";
      this.textBoxVerticalBin.Size = new System.Drawing.Size(55, 20);
      this.textBoxVerticalBin.TabIndex = 14;
      // 
      // textBoxHorizontalBin
      // 
      this.textBoxHorizontalBin.Location = new System.Drawing.Point(93, 113);
      this.textBoxHorizontalBin.Name = "textBoxHorizontalBin";
      this.textBoxHorizontalBin.Size = new System.Drawing.Size(55, 20);
      this.textBoxHorizontalBin.TabIndex = 13;
      // 
      // checkBoxLight
      // 
      this.checkBoxLight.AutoSize = true;
      this.checkBoxLight.Checked = true;
      this.checkBoxLight.CheckState = System.Windows.Forms.CheckState.Checked;
      this.checkBoxLight.Location = new System.Drawing.Point(93, 140);
      this.checkBoxLight.Name = "checkBoxLight";
      this.checkBoxLight.Size = new System.Drawing.Size(78, 17);
      this.checkBoxLight.TabIndex = 15;
      this.checkBoxLight.Text = "Light frame";
      this.checkBoxLight.UseVisualStyleBackColor = true;
      // 
      // textBoxDuration
      // 
      this.textBoxDuration.Location = new System.Drawing.Point(93, 163);
      this.textBoxDuration.Name = "textBoxDuration";
      this.textBoxDuration.Size = new System.Drawing.Size(55, 20);
      this.textBoxDuration.TabIndex = 16;
      // 
      // label5
      // 
      this.label5.AutoSize = true;
      this.label5.Location = new System.Drawing.Point(12, 166);
      this.label5.Name = "label5";
      this.label5.Size = new System.Drawing.Size(50, 13);
      this.label5.TabIndex = 17;
      this.label5.Text = "Duration:";
      // 
      // buttonStart
      // 
      this.buttonStart.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
      this.buttonStart.Location = new System.Drawing.Point(500, 90);
      this.buttonStart.Name = "buttonStart";
      this.buttonStart.Size = new System.Drawing.Size(72, 21);
      this.buttonStart.TabIndex = 18;
      this.buttonStart.Text = "Start";
      this.buttonStart.UseVisualStyleBackColor = true;
      this.buttonStart.Click += new System.EventHandler(this.buttonStart_Click);
      // 
      // buttonAbort
      // 
      this.buttonAbort.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
      this.buttonAbort.Location = new System.Drawing.Point(500, 117);
      this.buttonAbort.Name = "buttonAbort";
      this.buttonAbort.Size = new System.Drawing.Size(72, 21);
      this.buttonAbort.TabIndex = 19;
      this.buttonAbort.Text = "Abort";
      this.buttonAbort.UseVisualStyleBackColor = true;
      this.buttonAbort.Click += new System.EventHandler(this.buttonAbort_Click);
      // 
      // progressBarComplete
      // 
      this.progressBarComplete.Location = new System.Drawing.Point(93, 189);
      this.progressBarComplete.Name = "progressBarComplete";
      this.progressBarComplete.Size = new System.Drawing.Size(238, 23);
      this.progressBarComplete.Style = System.Windows.Forms.ProgressBarStyle.Continuous;
      this.progressBarComplete.TabIndex = 20;
      // 
      // label6
      // 
      this.label6.AutoSize = true;
      this.label6.Location = new System.Drawing.Point(12, 195);
      this.label6.Name = "label6";
      this.label6.Size = new System.Drawing.Size(60, 13);
      this.label6.TabIndex = 21;
      this.label6.Text = "Completed:";
      // 
      // labelState
      // 
      this.labelState.AutoSize = true;
      this.labelState.Location = new System.Drawing.Point(338, 194);
      this.labelState.Name = "labelState";
      this.labelState.Size = new System.Drawing.Size(0, 13);
      this.labelState.TabIndex = 22;
      // 
      // buttonSetup
      // 
      this.buttonSetup.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
      this.buttonSetup.Location = new System.Drawing.Point(500, 36);
      this.buttonSetup.Name = "buttonSetup";
      this.buttonSetup.Size = new System.Drawing.Size(72, 21);
      this.buttonSetup.TabIndex = 23;
      this.buttonSetup.Text = "Setup";
      this.buttonSetup.UseVisualStyleBackColor = true;
      this.buttonSetup.Click += new System.EventHandler(this.buttonSetup_Click);
      // 
      // TestForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(584, 675);
      this.Controls.Add(this.buttonSetup);
      this.Controls.Add(this.labelState);
      this.Controls.Add(this.label6);
      this.Controls.Add(this.progressBarComplete);
      this.Controls.Add(this.buttonAbort);
      this.Controls.Add(this.buttonStart);
      this.Controls.Add(this.label5);
      this.Controls.Add(this.textBoxDuration);
      this.Controls.Add(this.checkBoxLight);
      this.Controls.Add(this.textBoxVerticalBin);
      this.Controls.Add(this.textBoxHorizontalBin);
      this.Controls.Add(this.label3);
      this.Controls.Add(this.textBoxHeight);
      this.Controls.Add(this.textBoxWidth);
      this.Controls.Add(this.textBoxTop);
      this.Controls.Add(this.textBoxLeft);
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
    private System.Windows.Forms.Label label4;
    private System.Windows.Forms.TextBox textBoxLeft;
    private System.Windows.Forms.TextBox textBoxTop;
    private System.Windows.Forms.TextBox textBoxWidth;
    private System.Windows.Forms.TextBox textBoxHeight;
    private System.Windows.Forms.Label label3;
    private System.Windows.Forms.TextBox textBoxVerticalBin;
    private System.Windows.Forms.TextBox textBoxHorizontalBin;
    private System.Windows.Forms.CheckBox checkBoxLight;
    private System.Windows.Forms.TextBox textBoxDuration;
    private System.Windows.Forms.Label label5;
    private System.Windows.Forms.Button buttonStart;
    private System.Windows.Forms.Button buttonAbort;
    private System.Windows.Forms.ProgressBar progressBarComplete;
    private System.Windows.Forms.Label label6;
    private System.Windows.Forms.Label labelState;
    private System.Windows.Forms.Button buttonSetup;
  }
}


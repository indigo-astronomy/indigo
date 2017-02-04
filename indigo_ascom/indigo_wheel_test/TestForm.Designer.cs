namespace ASCOM.INDIGO
{
  partial class TestForm
  {
    /// <summary>
    /// Required designer variable.
    /// </summary>
    private System.ComponentModel.IContainer components = null;

    /// <summary>
    /// Clean up any resources being used.
    /// </summary>
    /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
    protected override void Dispose(bool disposing)
    {
      if (disposing && (components != null))
      {
        components.Dispose();
      }
      base.Dispose(disposing);
    }

    #region Windows Form Designer generated code

    /// <summary>
    /// Required method for Designer support - do not modify
    /// the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent()
    {
      this.buttonChoose = new System.Windows.Forms.Button();
      this.buttonConnect = new System.Windows.Forms.Button();
      this.labelDriverId = new System.Windows.Forms.Label();
      this.listBoxSlots = new System.Windows.Forms.ListBox();
      this.buttonGo = new System.Windows.Forms.Button();
      this.SuspendLayout();
      // 
      // buttonChoose
      // 
      this.buttonChoose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
      this.buttonChoose.Location = new System.Drawing.Point(325, 12);
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
      this.buttonConnect.Location = new System.Drawing.Point(325, 56);
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
      this.labelDriverId.Location = new System.Drawing.Point(12, 12);
      this.labelDriverId.Name = "labelDriverId";
      this.labelDriverId.Size = new System.Drawing.Size(307, 21);
      this.labelDriverId.TabIndex = 2;
      this.labelDriverId.Text = global::ASCOM.INDIGO.Properties.Settings.Default.DriverId;
      this.labelDriverId.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // listBoxSlots
      // 
      this.listBoxSlots.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.listBoxSlots.FormattingEnabled = true;
      this.listBoxSlots.Location = new System.Drawing.Point(12, 56);
      this.listBoxSlots.Name = "listBoxSlots";
      this.listBoxSlots.Size = new System.Drawing.Size(307, 134);
      this.listBoxSlots.TabIndex = 3;
      // 
      // buttonGo
      // 
      this.buttonGo.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
      this.buttonGo.Location = new System.Drawing.Point(325, 83);
      this.buttonGo.Name = "buttonGo";
      this.buttonGo.Size = new System.Drawing.Size(72, 21);
      this.buttonGo.TabIndex = 4;
      this.buttonGo.Text = "Go";
      this.buttonGo.UseVisualStyleBackColor = true;
      this.buttonGo.Click += new System.EventHandler(this.buttonGo_Click);
      // 
      // TestForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(409, 202);
      this.Controls.Add(this.buttonGo);
      this.Controls.Add(this.listBoxSlots);
      this.Controls.Add(this.labelDriverId);
      this.Controls.Add(this.buttonConnect);
      this.Controls.Add(this.buttonChoose);
      this.Name = "TestForm";
      this.Text = "INDIGO FilterWheel Tester";
      this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Form1_FormClosing);
      this.ResumeLayout(false);

    }

    #endregion

    private System.Windows.Forms.Button buttonChoose;
    private System.Windows.Forms.Button buttonConnect;
    private System.Windows.Forms.Label labelDriverId;
    private System.Windows.Forms.ListBox listBoxSlots;
    private System.Windows.Forms.Button buttonGo;
  }
}


namespace ASCOM.INDIGO
{
  partial class DeviceSelectionForm
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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DeviceSelectionForm));
      this.okButton = new System.Windows.Forms.Button();
      this.cancelButton = new System.Windows.Forms.Button();
      this.indigoLogo = new System.Windows.Forms.PictureBox();
      this.tree = new System.Windows.Forms.TreeView();
      ((System.ComponentModel.ISupportInitialize)(this.indigoLogo)).BeginInit();
      this.SuspendLayout();
      // 
      // okButton
      // 
      this.okButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
      this.okButton.DialogResult = System.Windows.Forms.DialogResult.OK;
      this.okButton.Enabled = false;
      this.okButton.Location = new System.Drawing.Point(250, 13);
      this.okButton.Name = "okButton";
      this.okButton.Size = new System.Drawing.Size(72, 23);
      this.okButton.TabIndex = 100;
      this.okButton.Text = "OK";
      this.okButton.UseVisualStyleBackColor = true;
      this.okButton.Click += new System.EventHandler(this.okClick);
      // 
      // cancelButton
      // 
      this.cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
      this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
      this.cancelButton.Location = new System.Drawing.Point(250, 42);
      this.cancelButton.Name = "cancelButton";
      this.cancelButton.Size = new System.Drawing.Size(72, 23);
      this.cancelButton.TabIndex = 101;
      this.cancelButton.Text = "Cancel";
      this.cancelButton.UseVisualStyleBackColor = true;
      this.cancelButton.Click += new System.EventHandler(this.cancelClick);
      // 
      // indigoLogo
      // 
      this.indigoLogo.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
      this.indigoLogo.Image = ((System.Drawing.Image)(resources.GetObject("indigoLogo.Image")));
      this.indigoLogo.Location = new System.Drawing.Point(250, 199);
      this.indigoLogo.Name = "indigoLogo";
      this.indigoLogo.Size = new System.Drawing.Size(72, 100);
      this.indigoLogo.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
      this.indigoLogo.TabIndex = 8;
      this.indigoLogo.TabStop = false;
      this.indigoLogo.Click += new System.EventHandler(this.browseToIndigo);
      // 
      // tree
      // 
      this.tree.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.tree.Location = new System.Drawing.Point(13, 13);
      this.tree.Name = "tree";
      this.tree.Size = new System.Drawing.Size(231, 286);
      this.tree.TabIndex = 102;
      this.tree.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.tree_AfterSelect);
      // 
      // DeviceSelectionForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(334, 311);
      this.Controls.Add(this.tree);
      this.Controls.Add(this.indigoLogo);
      this.Controls.Add(this.cancelButton);
      this.Controls.Add(this.okButton);
      this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
      this.MaximizeBox = false;
      this.MinimizeBox = false;
      this.Name = "DeviceSelectionForm";
      this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
      this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
      this.Text = "INDIGO Device Selection";
      ((System.ComponentModel.ISupportInitialize)(this.indigoLogo)).EndInit();
      this.ResumeLayout(false);

    }

    #endregion

    private System.Windows.Forms.Button okButton;
    private System.Windows.Forms.Button cancelButton;
    private System.Windows.Forms.PictureBox indigoLogo;
    private System.Windows.Forms.TreeView tree;
  }
}
namespace indigo_test {
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
      this.tree = new System.Windows.Forms.TreeView();
      this.SuspendLayout();
      // 
      // tree
      // 
      this.tree.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.tree.Location = new System.Drawing.Point(12, 12);
      this.tree.Name = "tree";
      this.tree.Size = new System.Drawing.Size(632, 373);
      this.tree.TabIndex = 0;
      this.tree.NodeMouseDoubleClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.tree_NodeMouseDoubleClick);
      // 
      // TestForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(656, 397);
      this.Controls.Add(this.tree);
      this.Name = "TestForm";
      this.Text = "INDIGO Test";
      this.ResumeLayout(false);

    }

    #endregion

    private System.Windows.Forms.TreeView tree;
  }
}


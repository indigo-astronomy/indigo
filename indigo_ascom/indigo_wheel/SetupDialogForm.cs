using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using INDIGO;

namespace ASCOM.INDIGO {
  [ComVisible(false)]
  public partial class SetupDialogForm : Form {

    private Client client;
    private Dictionary<Server, ServerNode> servers = new Dictionary<Server, ServerNode>();

    private class DeviceNode : TreeNode {
      private Device device;
      private TreeView tree;

      private void propertyAdded(Property property) {
        if (property.Name == "INFO") {
          foreach (Item item in property.Items) {
            if (item.Name == "DEVICE_INTERFACE") {
              int interfaceMask = Convert.ToInt32(((TextItem)item).Value);
              if ((interfaceMask & 16) == 16) {
                tree.BeginInvoke(new MethodInvoker(() => {
                  ForeColor = DefaultForeColor;
                  Parent.Expand();
                }));
              }
              break;
            }
          }
        }
      }

      public DeviceNode(TreeView tree, Device device) {
        this.tree = tree;
        this.device = device;
        Text = device.Name;
        ForeColor = System.Drawing.Color.LightGray;
        device.PropertyAdded += propertyAdded;
      }
    }

    private class ServerNode : TreeNode {
      private Server server;
      private TreeView tree;
      private Dictionary<Device, DeviceNode> devices = new Dictionary<Device, DeviceNode>();

      public ServerNode(TreeView tree, Server server) {
        this.tree = tree;
        this.server = server;
        Text = server.Name;
        ForeColor = System.Drawing.Color.LightGray;
        server.DeviceAdded += deviceAdded;
        server.DeviceRemoved += deviceRemoved;
      }

      private void deviceAdded(Device device) {
        DeviceNode node = new DeviceNode(tree, device);
        devices.Add(device, node);
        tree.BeginInvoke(new MethodInvoker(() => {
          tree.BeginUpdate();
          Nodes.Add(node);
          tree.EndUpdate();
        }));
      }

      private void deviceRemoved(Device device) {
        DeviceNode node;
        if (devices.TryGetValue(device, out node)) {
          devices.Remove(device);
          tree.BeginInvoke(new MethodInvoker(() => {
            tree.BeginUpdate();
            Nodes.Remove(node);
            tree.EndUpdate();
          }));
        }
      }
    }

    private void serverAdded(Server server) {
      server.ServerConnected += serverConnected;
      server.ServerDisconnected += serverDisconnected;
      ServerNode node = new ServerNode(tree, server);
      servers.Add(server, node);
      BeginInvoke(new MethodInvoker(() => {
        tree.BeginUpdate();
        tree.Nodes.Add(node);
        tree.EndUpdate();
      }));
    }

    private void serverConnected(Server server) {
      ServerNode node;
      if (servers.TryGetValue(server, out node)) {
        BeginInvoke(new MethodInvoker(() => {
          tree.BeginUpdate();
          node.ForeColor = DefaultForeColor;
          tree.EndUpdate();
        }));
      }
    }

    private void serverDisconnected(Server server) {
      ServerNode node;
      if (servers.TryGetValue(server, out node)) {
        BeginInvoke(new MethodInvoker(() => {
          tree.BeginUpdate();
          node.ForeColor = System.Drawing.Color.LightGray;
          node.Nodes.Clear();
          tree.EndUpdate();
        }));
      }
    }

    private void serverRemoved(Server server) {
      ServerNode node;
      if (servers.TryGetValue(server, out node)) {
        servers.Remove(server);
        BeginInvoke(new MethodInvoker(() => {
          tree.BeginUpdate();
          tree.Nodes.Remove(node);
          tree.EndUpdate();
        }));
      }
    }

    public SetupDialogForm() {
      InitializeComponent();
      client = new Client();
      client.ServerAdded += serverAdded;
      client.ServerRemoved += serverRemoved;
    }

    private void okClick(object sender, EventArgs e) {
      FilterWheel.indigoDevice = tree.SelectedNode.Text;
    }

    private void cancelClick(object sender, EventArgs e) {
      Close();
    }

    private void browseToIndigo(object sender, EventArgs e) {
      try {
        System.Diagnostics.Process.Start("http://www.indigo-astronomy.org");
      } catch (System.ComponentModel.Win32Exception noBrowser) {
        if (noBrowser.ErrorCode == -2147467259)
          MessageBox.Show(noBrowser.Message);
      } catch (System.Exception other) {
        MessageBox.Show(other.Message);
      }
    }

    private void tree_AfterSelect(object sender, TreeViewEventArgs e) {
      TreeNode node = ((TreeView)sender).SelectedNode;
      if (node is DeviceNode && (node.ForeColor == DefaultForeColor)) {
        okButton.Enabled = true;
      } else {
        okButton.Enabled = false;
      }
    }
  }
}
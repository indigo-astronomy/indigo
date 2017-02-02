using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using INDIGO;

namespace ASCOM.INDIGO {
  [ComVisible(false)]
  public partial class SetupDialogForm : Form {
    private FilterWheel driver;
    private Dictionary<Server, ServerNode> servers = new Dictionary<Server, ServerNode>();

    internal class DeviceNode : TreeNode {
      internal Device device;
      private SetupDialogForm form;

      private void propertyAdded(Property property) {
        if (property.Name == "INFO") {
          foreach (Item item in property.Items) {
            if (item.Name == "DEVICE_INTERFACE") {
              int interfaceMask = Convert.ToInt32(((TextItem)item).Value);
              if ((interfaceMask & 16) == 16) {
                form.BeginInvoke(new MethodInvoker(() => {
                  ForeColor = DefaultForeColor;
                  Parent.Expand();
                  if (form.driver.indigoDevice == property.DeviceName) {
                    form.tree.SelectedNode = this;
                    form.tree.Focus();
                  }
                }));
              }
              break;
            }
          }
        }
      }

      internal DeviceNode(SetupDialogForm tree, Device device) {
        this.form = tree;
        this.device = device;
        Text = device.Name;
        ForeColor = System.Drawing.Color.LightGray;
        device.PropertyAdded += propertyAdded;
      }
    }

    internal class ServerNode : TreeNode {
      private Server server;
      private SetupDialogForm form;
      private Dictionary<Device, DeviceNode> devices = new Dictionary<Device, DeviceNode>();

      internal ServerNode(SetupDialogForm tree, Server server) {
        this.form = tree;
        this.server = server;
        Text = server.Name;
        ForeColor = System.Drawing.Color.LightGray;
        server.DeviceAdded += deviceAdded;
        server.DeviceRemoved += deviceRemoved;
      }

      internal void deviceAdded(Device device) {
        if (devices.ContainsKey(device))
          return;
        DeviceNode node = new DeviceNode(form, device);
        devices.Add(device, node);
        form.BeginInvoke(new MethodInvoker(() => {
          form.tree.BeginUpdate();
          Nodes.Add(node);
          form.tree.EndUpdate();
        }));
      }

      private void deviceRemoved(Device device) {
        DeviceNode node;
        if (devices.TryGetValue(device, out node)) {
          devices.Remove(device);
          form.BeginInvoke(new MethodInvoker(() => {
            form.tree.BeginUpdate();
            Nodes.Remove(node);
            form.tree.EndUpdate();
          }));
        }
      }
    }

    private void serverAdded(Server server) {
      if (servers.ContainsKey(server))
        return;
      server.ServerConnected += serverConnected;
      server.ServerDisconnected += serverDisconnected;
      ServerNode node = new ServerNode(this, server);
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
          foreach (Device device in server.Devices) {
            node.deviceAdded(device);
          }
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

    public SetupDialogForm(FilterWheel driver) {
      InitializeComponent();
      this.driver = driver;
      Client client = driver.client;
      client.Mutex.WaitOne();
      foreach (Server server in client.Servers) {
        serverAdded(server);
      }
      client.Mutex.ReleaseMutex();
      client.ServerAdded += serverAdded;
      client.ServerRemoved += serverRemoved;
    }

    private void okClick(object sender, EventArgs e) {
      DeviceNode node = (DeviceNode)tree.SelectedNode;
      driver.indigoDevice = node.Text;
      driver.device = node.device;
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
        if (driver.connecting) {
          driver.indigoDevice = node.Text;
          driver.device = ((DeviceNode)node).device;
          Close();
        }
      } else {
        okButton.Enabled = false;
      }
    }
  }
}
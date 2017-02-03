using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using INDIGO;

namespace ASCOM.INDIGO {
  [ComVisible(false)]
  public partial class DeviceSelectionForm : Form {
    private BaseDriver driver;
    private string waitForDevice;
    private int interfaceMask;
    private Dictionary<Server, ServerNode> servers = new Dictionary<Server, ServerNode>();

    internal class DeviceNode : TreeNode {
      internal Device device;
      private DeviceSelectionForm form;

      private void propertyAdded(Property property) {
        if (property.Name == "INFO") {
          foreach (Item item in property.Items) {
            if (item.Name == "DEVICE_INTERFACE") {
              int interfaceMask = Convert.ToInt32(((TextItem)item).Value);
              if ((interfaceMask & form.interfaceMask) == form.interfaceMask) {
                form.BeginInvoke(new MethodInvoker(() => {
                  ForeColor = DefaultForeColor;
                  Parent.Expand();
                  if (form.driver.deviceName == property.DeviceName) {
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

      internal DeviceNode(DeviceSelectionForm tree, Device device) {
        this.form = tree;
        this.device = device;
        Text = device.Name;
        ForeColor = System.Drawing.Color.LightGray;
        device.PropertyAdded += propertyAdded;
      }
    }

    internal class ServerNode : TreeNode {
      private Server server;
      private DeviceSelectionForm form;
      private Dictionary<Device, DeviceNode> devices = new Dictionary<Device, DeviceNode>();

      internal ServerNode(DeviceSelectionForm tree, Server server) {
        this.form = tree;
        this.server = server;
        Text = server.Name;
        ForeColor = System.Drawing.Color.LightGray;
        server.DeviceAdded += deviceAdded;
        server.DeviceRemoved += deviceRemoved;
      }

      internal void deviceAdded(Device device) {
        try {
          DeviceNode node = new DeviceNode(form, device);
          devices.Add(device, node);
          form.BeginInvoke(new MethodInvoker(() => {
            form.tree.BeginUpdate();
            Nodes.Add(node);
            form.tree.EndUpdate();
          }));
        } catch {
        }
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
      try {
        server.ServerConnected += serverConnected;
        server.ServerDisconnected += serverDisconnected;
        ServerNode node = new ServerNode(this, server);
        servers.Add(server, node);
        BeginInvoke(new MethodInvoker(() => {
          tree.BeginUpdate();
          tree.Nodes.Add(node);
          tree.EndUpdate();
        }));
      } catch {
      }
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

    public DeviceSelectionForm(BaseDriver driver, string waitForDevice) {
      InitializeComponent();
      this.driver = driver;
      this.waitForDevice = waitForDevice;
      interfaceMask = (int)driver.deviceInterface;
      if (waitForDevice == string.Empty)
        Text = "INDIGO " + driver.deviceInterface.ToString("g") + " selection";
      else
        Text = "Waiting for " + waitForDevice;
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
      driver.deviceName = node.Text;
      driver.device = node.device;
    }

    private void cancelClick(object sender, EventArgs e) {
      Close();
    }

    private void browseToIndigo(object sender, EventArgs e) {
      try {
        System.Diagnostics.Process.Start("http://www.indigo-astronomy.org");
      } catch {
      }
    }

    private void tree_AfterSelect(object sender, TreeViewEventArgs e) {
      okButton.Enabled = false;
      if (((TreeView)sender).SelectedNode is DeviceNode) {
        DeviceNode node = (DeviceNode)((TreeView)sender).SelectedNode;
        if (node.ForeColor == DefaultForeColor) {
          if (node.Text == waitForDevice) {
            driver.deviceName = node.Text;
            driver.device = ((DeviceNode)node).device;
            Close();
          } else {
            okButton.Enabled = true;
          }
        }
      }
    }
  }
}
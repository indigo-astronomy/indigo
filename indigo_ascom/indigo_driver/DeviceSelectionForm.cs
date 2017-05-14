using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using INDIGO;
using System.Drawing;

namespace ASCOM.INDIGO {
  [ComVisible(false)]
  public partial class DeviceSelectionForm : Form {
    private BaseDriver driver;
    private int interfaceMask;
    private Dictionary<Server, ServerNode> servers = new Dictionary<Server, ServerNode>();

    internal class DeviceNode : TreeNode {
      internal Device device;
      private DeviceSelectionForm form;

      internal void propertyAdded(Property property) {
        if (property.Name == "INFO") {
          foreach (Item item in property.Items) {
            if (item.Name == "DEVICE_INTERFACE") {
              int interfaceMask = Convert.ToInt32(((TextItem)item).Value);
              if ((interfaceMask & form.interfaceMask) == form.interfaceMask) {
                if (!form.Visible) {
                  ForeColor = DefaultForeColor;
                  Parent.Expand();
                  if (form.driver.deviceName == property.DeviceName) {
                    form.tree.SelectedNode = this;
                    form.tree.Focus();
                  }
                } else {
                  form.BeginInvoke(new MethodInvoker(() => {
                    ForeColor = DefaultForeColor;
                    Parent.Expand();
                    if (form.driver.deviceName == property.DeviceName) {
                      form.tree.SelectedNode = this;
                      form.tree.Focus();
                    }
                  }));
                }
              }
              break;
            }
          }
        }
      }

      internal DeviceNode(DeviceSelectionForm form, Device device) {
        this.form = form;
        this.device = device;
        Text = device.Name;
        ForeColor = System.Drawing.Color.LightGray;
        device.PropertyAdded += propertyAdded;
      }

      internal void removeEvenHandlers() {
        device.PropertyAdded -= propertyAdded;
      }
    }

    internal class ServerNode : TreeNode {
      public Server server;
      private DeviceSelectionForm form;
      private Dictionary<Device, DeviceNode> devices = new Dictionary<Device, DeviceNode>();

      internal ServerNode(DeviceSelectionForm form, Server server) {
        this.form = form;
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
          if (!form.Visible) {
            Nodes.Add(node);
          } else {
            form.BeginInvoke(new MethodInvoker(() => {
              form.tree.BeginUpdate();
              Nodes.Add(node);
              form.tree.EndUpdate();
            }));
          }
          foreach (Property property in device.Properties)
            node.propertyAdded(property);
        } catch  (Exception exception) {
          Console.WriteLine(exception);
        }
      }

      internal void deviceRemoved(Device device) {
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

      internal void removeEventHandlers() {
        server.DeviceAdded -= deviceAdded;
        server.DeviceRemoved -= deviceRemoved;
        foreach (TreeNode node in Nodes) {
          ((DeviceNode)node).removeEvenHandlers();
        }
      }
    }

    private void serverAdded(Server server) {
      try {
        server.ServerConnected += serverConnected;
        server.ServerDisconnected += serverDisconnected;
        ServerNode node = new ServerNode(this, server);
        servers.Add(server, node);
        if (!Visible) {
          tree.Nodes.Add(node);
        } else {
          BeginInvoke(new MethodInvoker(() => {
            tree.BeginUpdate();
            tree.Nodes.Add(node);
            tree.EndUpdate();
          }));
        }
        foreach (Device device in server.Devices) {
          node.deviceAdded(device);
        }
      } catch (Exception exception) {
        Console.WriteLine(exception);
      }
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

    public DeviceSelectionForm(BaseDriver driver) {
      InitializeComponent();
      this.driver = driver;
      interfaceMask = (int)driver.deviceInterface;
      Text = "INDIGO " + driver.deviceInterface.ToString("g") + " Selector";
      Client client = BaseDriver.client;
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
      removeEventHandlers();
    }

    private void cancelClick(object sender, EventArgs e) {
      removeEventHandlers();
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
          okButton.Enabled = true;
        }
      }
    }

    private void removeEventHandlers() {
      Client client = BaseDriver.client;
      client.ServerAdded -= serverAdded;
      client.ServerRemoved -= serverRemoved;
      foreach (TreeNode node in tree.Nodes) {
        ServerNode serverNode = (ServerNode)node;
        Server server = serverNode.server;
        server.ServerConnected -= serverConnected;
        server.ServerDisconnected -= serverDisconnected;
        serverNode.removeEventHandlers();
      }
    }
  }
}

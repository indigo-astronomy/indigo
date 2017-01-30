using INDIGO;
using System.Collections.Generic;
using System.Windows.Forms;

namespace indigo_test {

  public partial class TestForm : Form {
    private Client client;
    private Dictionary<Server, ServerNode> servers = new Dictionary<Server, ServerNode>();

    private class ItemNode : TreeNode {
      private Item item;

      public void update() {
        if (item is SwitchItem)
          Text = "SWITCH '" + item.name + "' label = '" + item.label + "' value = " + ((SwitchItem)item).value;
        else if (item is TextItem)
          Text = "TEXT '" + item.name + "' label = '" + item.label + "' value = " + ((TextItem)item).value;
        else if (item is NumberItem)
          Text = "NUMBER '" + item.name + "' label = '" + item.label + "' value = " + ((NumberItem)item).value + " min = " + ((NumberItem)item).min + " max = " + ((NumberItem)item).max + " step = " + ((NumberItem)item).step + " format = '" + ((NumberItem)item).format + "'";
        else if (item is LightItem)
          Text = "LIGHT '" + item.name + "' label = '" + item.label + "' value = " + ((LightItem)item).value;
        else if (item is BLOBItem)
          Text = "BLOB '" + item.name + "' label = '" + item.label + "' value = " + ((BLOBItem)item).value;
      }

      public ItemNode(Item item) {
        this.item = item;
        update();
      }
    }

    private class PropertyNode : TreeNode {
      private Property property;
      private TreeView tree;

      public PropertyNode(TreeView tree, Property property) {
        this.tree = tree;
        this.property = property;
        Text = "PROPERTY '" + property.name + "' label = '" + property.label + "' state = " + property.state + " perm = " + property.perm;
        if (property is SwitchProperty)
          Text += " rule = " + ((SwitchProperty)property).rule;
        foreach (Item item in property.items) {
          Nodes.Add(new ItemNode(item));
        }
      }
    }

    private class GroupNode : TreeNode {
      private Group group;
      private TreeView tree;
      private Dictionary<Property, PropertyNode> properties = new Dictionary<Property, PropertyNode>();

      public GroupNode(TreeView tree, Group group) {
        this.tree = tree;
        this.group = group;
        Text = "GROUP '" + group.group + "'";
        group.propertyAdded += propertyAdded;
        group.propertyRemoved += propertyRemoved;
      }

      private void propertyAdded(Property property) {
        PropertyNode node = new PropertyNode(tree, property);
        properties.Add(property, node);
        tree.BeginInvoke(new MethodInvoker(() => {
          tree.BeginUpdate();
          Nodes.Add(node);
          tree.EndUpdate();
        }));
      }

      private void propertyRemoved(Property property) {
        PropertyNode node;
        if (properties.TryGetValue(property, out node)) {
          properties.Remove(property);
          tree.BeginInvoke(new MethodInvoker(() => {
            tree.BeginUpdate();
            Nodes.Remove(node);
            tree.EndUpdate();
          }));
        }
      }
    }

    private class DeviceNode : TreeNode {
      private Device device;
      private TreeView tree;
      private Dictionary<Group, GroupNode> groups = new Dictionary<Group, GroupNode>();

      public DeviceNode(TreeView tree, Device device) {
        this.tree = tree;
        this.device = device;
        Text = "DEVICE '" + device.device + "'";
        device.groupAdded += groupAdded;
        device.groupRemoved += groupRemoved;
      }

      private void groupAdded(Group group) {
        GroupNode node = new GroupNode(tree, group);
        groups.Add(group, node);
        tree.BeginInvoke(new MethodInvoker(() => {
          tree.BeginUpdate();
          Nodes.Add(node);
          tree.EndUpdate();
        }));
      }

      private void groupRemoved(Group group) {
        GroupNode node;
        if (groups.TryGetValue(group, out node)) {
          groups.Remove(group);
          tree.BeginInvoke(new MethodInvoker(() => {
            tree.BeginUpdate();
            Nodes.Remove(node);
            tree.EndUpdate();
          }));
        }
      }
    }

    private class ServerNode : TreeNode {
      private Server server;
      private TreeView tree;
      private Dictionary<Device, DeviceNode> devices = new Dictionary<Device, DeviceNode>();

      public ServerNode(TreeView tree, Server server) {
        this.tree = tree;
        this.server = server;
        Text = "SERVER '" + server.name + "'";
        server.deviceAdded += deviceAdded;
        server.deviceRemoved += deviceRemoved;
      }

      private void deviceAdded(Device device) {
        DeviceNode node = new DeviceNode(tree, device);
        devices.Add(device, node);
        tree.BeginInvoke(new MethodInvoker(() => {
          tree.BeginUpdate();
          Nodes.Add(node);
          tree.EndUpdate();
          Expand();
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
      ServerNode node = new ServerNode(tree, server);
      servers.Add(server, node);
      BeginInvoke(new MethodInvoker(() => {
        tree.BeginUpdate();
        tree.Nodes.Add(node);
        tree.EndUpdate();
      }));
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

    public TestForm() {
      InitializeComponent();
      client = new Client();
      client.serverAdded += serverAdded;
      client.serverRemoved += serverRemoved;
    }
  }
}

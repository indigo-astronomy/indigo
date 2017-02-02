using INDIGO;
using System;
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
          Text = "SWITCH '" + item.Name + "' label = '" + item.Label + "' value = " + ((SwitchItem)item).Value;
        else if (item is TextItem)
          Text = "TEXT '" + item.Name + "' label = '" + item.Label + "' value = " + ((TextItem)item).Value;
        else if (item is NumberItem)
          Text = "NUMBER '" + item.Name + "' label = '" + item.Label + "' value = " + ((NumberItem)item).Value + " target = " + ((NumberItem)item).Target + " min = " + ((NumberItem)item).Min + " max = " + ((NumberItem)item).Max + " step = " + ((NumberItem)item).Step + " format = '" + ((NumberItem)item).Format + "'";
        else if (item is LightItem)
          Text = "LIGHT '" + item.Name + "' label = '" + item.Label + "' value = " + ((LightItem)item).Value;
        else if (item is BLOBItem)
          Text = "BLOB '" + item.Name + "' label = '" + item.Label + "' value = " + ((BLOBItem)item).Value;
      }

      public void Action() {
        if (item is SwitchItem) {
          SwitchItem switchItem = (SwitchItem)item;
          switchItem.Value = !switchItem.Value;
          item.Property.Change();
        }
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
        Text = "PROPERTY '" + property.Name + "' label = '" + property.Label + "' state = " + property.State + " perm = " + property.Permission;
        if (property is SwitchProperty)
          Text += " rule = " + ((SwitchProperty)property).Rule;
        foreach (Item item in property.Items) {
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
        Text = "GROUP '" + group.Name + "'";
        group.PropertyAdded += propertyAdded;
        group.PropertyUpdated += propertyChanged;
        group.PropertyRemoved += propertyRemoved;
      }

      private void updateState(PropertyNode node, Property.States state) {
        if (state == Property.States.Ok)
          node.ForeColor = System.Drawing.Color.Green;
        else if (state == Property.States.Busy)
          node.ForeColor = System.Drawing.Color.DarkOrange;
        else if (state == Property.States.Alert)
          node.ForeColor = System.Drawing.Color.Red;
        else
          node.ForeColor = DefaultForeColor;
        if (state != Property.States.Idle) {
          node.Parent.Parent.Parent.Expand();
          node.Parent.Parent.Expand();
          node.Parent.Expand();
          node.Expand();
        }
      }

      private void propertyAdded(Property property) {
        PropertyNode node = new PropertyNode(tree, property);
        properties.Add(property, node);
        tree.BeginInvoke(new MethodInvoker(() => {
          tree.BeginUpdate();
          Nodes.Add(node);
          updateState(node, property.State);
          tree.EndUpdate();
        }));
      }

      private void propertyChanged(Property property) {
        PropertyNode node;
        if (properties.TryGetValue(property, out node)) {
          tree.BeginInvoke(new MethodInvoker(() => {
            tree.BeginUpdate();
            foreach (TreeNode item in node.Nodes)
              ((ItemNode)item).update();
            updateState(node, property.State);
            tree.EndUpdate();
          }));
        }
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
        Text = "DEVICE '" + device.Name + "'";
        device.GroupAdded += groupAdded;
        device.GroupRemoved += groupRemoved;
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
        Text = "SERVER '" + server.Name + "'";
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
        node.ForeColor = System.Drawing.Color.LightGray;
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

    public TestForm() {
      InitializeComponent();
      client = new Client();
      client.ServerAdded += serverAdded;
      client.ServerRemoved += serverRemoved;
    }

    private void tree_NodeMouseDoubleClick(object sender, TreeNodeMouseClickEventArgs e) {
      TreeView tree = (TreeView)sender;
      TreeNode node = tree.SelectedNode;
      Console.WriteLine(node);
      if (node is ItemNode)
        ((ItemNode)node).Action();
      
    }
  }
}

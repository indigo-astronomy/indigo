//
// INDIGO for .Net
//
// Copyright (c) 2017 CloudMakers, s. r. o.
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

using System;
using System.Threading;
using System.Text;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Net.WebSockets;
using Bonjour;
using System.Threading.Tasks;
using ASCOM.Utilities;

#pragma warning disable CS0649 // Field is never assigned to, and will always have its default value null

namespace INDIGO {
  public delegate void ServerAddedHandler(Server server);
  public delegate void ServerRemovedHandler(Server server);
  public delegate void ServerConnectedHandler(Server server);
  public delegate void ServerDisconnectedHandler(Server server);
  public delegate void DeviceAddedHandler(Device device);
  public delegate void DeviceRemovedHandler(Device device);
  public delegate void GroupAddedHandler(Group group);
  public delegate void GroupRemovedHandler(Group group);
  public delegate void PropertyAddedHandler(Property property);
  public delegate void PropertyUpdatedHandler(Property property);
  public delegate void PropertyRemovedHandler(Property property);

  [DataContract]
  public class Indigo {
    private static TraceLogger logger = new TraceLogger("", "INDIGO") { Enabled = true };

    public void Log(string message) {
      logger.LogMessage(GetType().ToString(), message);
    }
  }

  [DataContract]
  public class Item : Indigo {
    private Property property;
    [DataMember(Name = "name")]
    private string name;
    [DataMember(Name = "label")]
    private string label;

    [DataContract]
    internal class ChangeItem {
      [DataMember(Name = "name")]
      internal string name;
    }

    virtual internal ChangeItem NewChangeItem() {
      return null;
    }

    public string Name {
      get {
        return name;
      }
    }

    public string Label {
      get {
        return label;
      }
    }

    public Property Property {
      get {
        return property;
      }

      set {
        if (property == null)
          property = value;
      }
    }

    virtual internal void Update(Item item) {
    }

    override public bool Equals(object other) {
      return Name.Equals(((Item)other).Name);
    }

    override public int GetHashCode() {
      return name.GetHashCode();
    }
  }

  [DataContract]
  public class SwitchItem : Item {
    [DataMember(Name = "value")]
    private bool value;
    private bool modifiedValue;
    private bool modified = false;

    [DataContract]
    internal class ChangeSwitchItem : ChangeItem {
      [DataMember(Name = "value")]
      internal bool value;
    }

    override internal ChangeItem NewChangeItem() {
      if (!modified)
        return null;
      modified = false;
      return new ChangeSwitchItem() { name = this.Name, value = modifiedValue };
    }

    public bool Value {
      get {
        return value;
      }

      set {
        modifiedValue = value;
        modified = true;
      }
    }

    override internal void Update(Item item) {
      value = ((SwitchItem)item).value;
    }

    public override string ToString() {
      return Name + "=" + value;
    }
  }

  [DataContract]
  public class TextItem : Item {
    [DataMember(Name = "value")]
    private string value;
    private string modifiedValue;
    private bool modified = false;

    [DataContract]
    internal class ChangeTextItem : ChangeItem {
      [DataMember(Name = "value")]
      internal string value;
    }

    override internal ChangeItem NewChangeItem() {
      if (!modified)
        return null;
      modified = false;
      return new ChangeTextItem() { name = this.Name, value = modifiedValue };
    }

    public string Value {
      get {
        return value;
      }

      set {
        modifiedValue = value;
        modified = true;
      }
    }

    override internal void Update(Item item) {
      value = ((TextItem)item).value;
    }

    public override string ToString() {
      return Name + "='" + value + "'";
    }
  }

  [DataContract]
  public class NumberItem : Item {
    [DataMember(Name = "min")]
    private double min;
    [DataMember(Name = "max")]
    private double max;
    [DataMember(Name = "step")]
    private double step;
    [DataMember(Name = "target")]
    private double target;
    [DataMember(Name = "format")]
    private string format;
    [DataMember(Name = "value")]
    private double value;
    private double modifiedValue;
    private bool modified = false;

    [DataContract]
    internal class ChangeNumberItem : ChangeItem {
      [DataMember(Name = "value")]
      internal double value;
    }

    override internal ChangeItem NewChangeItem() {
      if (!modified)
        return null;
      modified = false;
      return new ChangeNumberItem() { name = this.Name, value = modifiedValue };
    }

    public double Min {
      get {
        return min;
      }
    }

    public double Max {
      get {
        return max;
      }
    }

    public double Step {
      get {
        return step;
      }
    }

    public double Target {
      get {
        return target;
      }
    }

    public string Format {
      get {
        return format;
      }
    }

    public double Value {
      get {
        return value;
      }

      set {
        modifiedValue = value;
        modified = true;
      }
    }

    override internal void Update(Item item) {
      value = ((NumberItem)item).value;
      target = ((NumberItem)item).target;
    }

    public override string ToString() {
      return Name + "=" + value;
    }
  }

  [DataContract]
  public class LightItem : Item {
    [DataMember(Name = "value")]
    private string value;

    public string Value {
      get {
        return value;
      }
    }

    override internal void Update(Item item) {
      value = ((LightItem)item).value;
    }

    public override string ToString() {
      return Name + "=" + value;
    }
  }

  [DataContract]
  public class BLOBItem : Item {
    [DataMember(Name = "value")]
    private string value;

    public string Value {
      get {
        return value;
      }
    }

    override internal void Update(Item item) {
      value = ((BLOBItem)item).value;
    }

    public override string ToString() {
      return Name + "='" + value + "'";
    }
  }

  [DataContract]
  public class Property: Indigo {

    public enum Permissions {
      ReadOnly, WriteOnly, ReadWrite
    }

    public enum States {
      Idle, Busy, Alert, Ok
    }

    private Device device;
    private Group group;
    [DataMember(Name = "version")]
    private int version;
    [DataMember(Name = "device")]
    private string deviceName;
    [DataMember(Name = "name")]
    private string name;
    [DataMember(Name = "group")]
    private string groupName;
    [DataMember(Name = "label")]
    private string label;
    [DataMember(Name = "perm")]
    private string permission;
    [DataMember(Name = "state")]
    private string state;
    [DataMember(Name = "message")]
    private string message;

    [DataContract]
    internal class ChangeProperty {
      [DataMember(Name = "device")]
      internal string device;
      [DataMember(Name = "name")]
      internal string name;
      [DataMember(Name = "items")]
      internal List<Item.ChangeItem> items;
    }

    [DataContract]
    [KnownType(typeof(SwitchItem.ChangeSwitchItem))]
    [KnownType(typeof(TextItem.ChangeTextItem))]
    [KnownType(typeof(NumberItem.ChangeNumberItem))]
    internal class ChangeMessage {
      [DataMember(Name = "newSwitchVector", EmitDefaultValue = false)]
      internal ChangeProperty newSwitchVector;
      [DataMember(Name = "newTextVector", EmitDefaultValue = false)]
      internal ChangeProperty newTextVector;
      [DataMember(Name = "newNumberVector", EmitDefaultValue = false)]
      internal ChangeProperty newNumberVector;
    }

    virtual internal ChangeMessage NewChangeMessage() {
      return null;
    }

    public virtual IReadOnlyList<Item> Items {
      get {
        return null;
      }
    }

    public Device Device {

      get {
        return device;
      }

      set {
        if (device == null)
          device = value;
      }
    }

    public Group Group {
      get {
        return group;
      }

      set {
        if (group == null)
          group = value;
      }
    }

    public int Version {
      get {
        return version;
      }
    }

    public string DeviceName {
      get {
        return deviceName;
      }
    }

    public string Name {
      get {
        return name;
      }
    }

    public string GroupName {
      get {
        return groupName;
      }
    }

    public string Label {
      get {
        return label;
      }
    }

    public Permissions Permission {
      get {
        switch (permission) {
          case "wo":
            return Permissions.WriteOnly;
          case "rw":
            return Permissions.ReadWrite;
        }
        return Permissions.ReadOnly;
      }
    }

    public States State {
      get {
        switch (state) {
          case "Busy":
            return States.Busy;
          case "Alert":
            return States.Alert;
          case "Ok":
            return States.Ok;
        }
        return States.Idle;
      }
    }

    public string Message {
      get {
        return message;
      }
    }

    public Item GetItem(string name) {
      return Items.FirstOrDefault(x => x.Name == name);
    }

    public void Update(Property property) {
      state = property.state;
      message = property.message;
      foreach (Item item in property.Items) {
        Item cachedItem = Items.FirstOrDefault(x => x.Name == item.Name);
        if (cachedItem != null) {
          cachedItem.Update(item);
          break;
        }
      }
    }

    override public bool Equals(object other) {
      return Name.Equals(((Property)other).Name);
    }

    override public int GetHashCode() {
      return name.GetHashCode();
    }

    private static DataContractJsonSerializerSettings settings = new DataContractJsonSerializerSettings() {
      EmitTypeInformation = EmitTypeInformation.Never
    };

    virtual public void Change() {
      ChangeMessage message = NewChangeMessage();
      if (message == null)
        return;

      Server server = device.Server;
      MemoryStream stream = new MemoryStream();
      DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(ChangeMessage), settings);
      serializer.WriteObject(stream, message);
      String json = System.Text.Encoding.Default.GetString(stream.GetBuffer());
      server.SendMessage(json);
    }

    public override string ToString() {
      IEnumerator<Item> e = Items.GetEnumerator();
      StringBuilder s = new StringBuilder("'" + deviceName + "'.'" + name + "' " + state + " { ");
      if (e.MoveNext()) {
        s.Append(e.Current);
        while (e.MoveNext()) {
          s.Append(", " + e.Current);
        }
      }
      s.Append(" }");
      return s.ToString();
    }
  }

  [DataContract]
  public class SwitchProperty : Property {

    public enum Rules {
      AnyOfMany, OneOfMany, AtMostOne
    }

    [DataMember(Name = "rule")]
    private string rule;
    [DataMember(Name = "items")]
    private List<SwitchItem> items;

    override internal ChangeMessage NewChangeMessage() {
      List<Item.ChangeItem> changeItems = new List<Item.ChangeItem>();
      foreach (Item item in items) {
        Item.ChangeItem changeItem = item.NewChangeItem();
        if (changeItem != null)
          changeItems.Add(changeItem);
      }
      if (changeItems.Count == 0)
        return null;
      return new ChangeMessage() { newSwitchVector = new ChangeProperty() { device = DeviceName, name = Name, items = changeItems } };
    }

    override public IReadOnlyList<Item> Items {
      get {
        return items.Cast<Item>().ToList().AsReadOnly();
      }
    }

    public Rules Rule {
      get {
        switch (rule) {
          case "OneOfMany":
            return Rules.OneOfMany;
          case "AtMostOne":
            return Rules.AtMostOne;
        }
        return Rules.AnyOfMany;
      }
    }

    public void SetSingleValue(string name, bool value) {
      if (rule != "AnyOfMany") {
        foreach (SwitchItem item in items)
          item.Value = false;
      }
      foreach (SwitchItem item in items)
        if (item.Name == name)
          item.Value = value;
      Change();
    }

    public override string ToString() {
      return "Switch " + base.ToString();
    }
  }

  [DataContract]
  public class TextProperty : Property {
    [DataMember(Name = "items")]
    private List<TextItem> items;

    override internal ChangeMessage NewChangeMessage() {
      List<Item.ChangeItem> changeItems = new List<Item.ChangeItem>();
      foreach (Item item in items) {
        Item.ChangeItem changeItem = item.NewChangeItem();
        if (changeItem != null)
          changeItems.Add(changeItem);
      }
      if (changeItems.Count == 0)
        return null;
      return new ChangeMessage() { newTextVector = new ChangeProperty() { device = DeviceName, name = Name, items = changeItems } };
    }

    override public IReadOnlyList<Item> Items {
      get {
        return items.Cast<Item>().ToList().AsReadOnly();
      }
    }

    public void SetSingleValue(string name, string value) {
      TextItem item = items.FirstOrDefault(x => x.Name == name);
      if (item != null) {
        item.Value = value;
        Change();
      }
    }

    public override string ToString() {
      return "Text " + base.ToString();
    }
  }

  [DataContract]
  public class NumberProperty : Property {
    [DataMember(Name = "items")]
    private List<NumberItem> items;

    override internal ChangeMessage NewChangeMessage() {
      List<Item.ChangeItem> changeItems = new List<Item.ChangeItem>();
      foreach (Item item in items) {
        Item.ChangeItem changeItem = item.NewChangeItem();
        if (changeItem != null)
          changeItems.Add(changeItem);
      }
      if (changeItems.Count == 0)
        return null;
      return new ChangeMessage() { newNumberVector = new ChangeProperty() { device = DeviceName, name = Name, items = changeItems } };
    }

    override public IReadOnlyList<Item> Items {
      get {
        return items.Cast<Item>().ToList().AsReadOnly();
      }
    }

    public void SetSingleValue(string name, double value) {
      NumberItem item = items.FirstOrDefault(x => x.Name == name);
      if (item != null) {
        item.Value = value;
        Change();
      }
    }

    public override string ToString() {
      return "Number " + base.ToString();
    }
  }

  [DataContract]
  public class LightProperty : Property {
    [DataMember(Name = "items")]
    private List<LightItem> items;

    override public IReadOnlyList<Item> Items {
      get {
        return items.Cast<Item>().ToList().AsReadOnly();
      }
    }

    public override string ToString() {
      return "Light " + base.ToString();
    }
  }

  [DataContract]
  public class BLOBProperty : Property {
    [DataMember(Name = "items")]
    private List<BLOBItem> items;

    override public IReadOnlyList<Item> Items {
      get {
        return items.Cast<Item>().ToList().AsReadOnly();
      }
    }

    public override string ToString() {
      return "BLOB " + base.ToString();
    }
  }

  public class Group: Indigo {
    private string name;
    private List<Property> properties = new List<Property>();
    private Device parentDevice;

    public event PropertyAddedHandler PropertyAdded;
    public event PropertyUpdatedHandler PropertyUpdated;
    public event PropertyRemovedHandler PropertyRemoved;

    public string Name {
      get {
        return name;
      }
    }

    public IReadOnlyList<Property> Properties {
      get {
        return properties.AsReadOnly();
      }
    }

    public Device ParentDevice {
      get {
        return parentDevice;
      }
    }

    public void Add(Property property) {
      if (properties.Contains(property))
        return;
      property.Group = this;
      properties.Add(property);
      PropertyAdded?.Invoke(property);
    }

    public void Update(Property property) {
      Property cachedProperty = properties.FirstOrDefault(x => x.Name == property.Name);
      if (cachedProperty != null) {
        cachedProperty.Update(property);
        PropertyUpdated?.Invoke(cachedProperty);
        return;
      }
    }

    public void Delete(Property property) {
      Property cachedProperty = properties.FirstOrDefault(x => x.Name == property.Name);
      if (cachedProperty != null) {
        properties.Remove(cachedProperty);
        PropertyRemoved?.Invoke(cachedProperty);
      }
    }

    public Group(string name, Device parentDevice) {
      this.name = name;
      this.parentDevice = parentDevice;
    }

    override public bool Equals(object other) {
      return Name.Equals(((Group)other).Name);
    }

    override public int GetHashCode() {
      return name.GetHashCode();
    }
  }

  public class Device: Indigo {

    public enum InterfaceMask {
      Mount = 1 << 0,
      CCD = 1 << 1,
      Guider = 1 << 2,
      Focuser = 1 << 3,
      FilterWheel = 1 << 4,
      Dome = 1 << 5
    }

    private string name;
    private Server server;
    private Dictionary<string, Group> groups = new Dictionary<string, Group>();
    private List<Property> properties = new List<Property>();

    public event GroupAddedHandler GroupAdded;
    public event GroupRemovedHandler GroupRemoved;

    public event PropertyAddedHandler PropertyAdded;
    public event PropertyUpdatedHandler PropertyUpdated;
    public event PropertyRemovedHandler PropertyRemoved;

    public string Name {
      get {
        return name;
      }
    }

    public IReadOnlyList<Group> Groups {
      get {
        return groups.Values.Cast<Group>().ToList().AsReadOnly();
      }
    }

    public IReadOnlyList<Property> Properties {
      get {
        return properties.AsReadOnly();
      }
    }

    public Server Server {
      get {
        return server;
      }
    }

    private Group GetOrAddGroup(string name) {
      Group group = null;
      if (!groups.TryGetValue(name, out group)) {
        group = new Group(name, this);
        groups.Add(group.Name, group);
        GroupAdded?.Invoke(group);
      }
      return group;
    }

    public Property GetProperty(string name) {
      return properties.FirstOrDefault(x => x.Name == name);
    }

    public void Add(Property property) {
      if (properties.Contains(property))
        return;
      property.Device = this;
      foreach (Item item in property.Items)
        item.Property = property;
      properties.Add(property);
      PropertyAdded?.Invoke(property);
      Group group = GetOrAddGroup(property.GroupName);
      group.Add(property);
    }

    public void Update(Property property) {
      Property cachedProperty = properties.FirstOrDefault(x => x.Name == property.Name);
      if (cachedProperty != null) {
        cachedProperty.Update(property);
        PropertyUpdated?.Invoke(cachedProperty);
        GetOrAddGroup(cachedProperty.GroupName).Update(property);
        return;
      }
      Log("'" + property.DeviceName + "' '" + property.Name + "' not found!");
    }

    public void Delete(Property property) {
      if (property.Name == null) {
        properties.Clear();
        groups.Clear();
      } else {
        Property cachedProperty = properties.FirstOrDefault(x => x.Name == property.Name);
        if (cachedProperty != null) {
          properties.Remove(cachedProperty);
          PropertyRemoved?.Invoke(cachedProperty);
          Group group = GetOrAddGroup(cachedProperty.GroupName);
          group.Delete(property);
          if (group.Properties.Count == 0) {
            groups.Remove(group.Name);
            GroupRemoved?.Invoke(group);
          }
        }
      }
    }

    public Device() {
    }

    public Device(string name, Server parentServer) {
      this.name = name;
      this.server = parentServer;
    }

    public void GetProperties() {
      server.SendMessage("{ 'getProperties': { 'version': 512, 'device': '" + name + "' } }");
    }

    override public bool Equals(object other) {
      return Name.Equals(((Device)other).Name);
    }

    override public int GetHashCode() {
      return name.GetHashCode();
    }
  }

  public class Server: Indigo {
    private Client client;
    private Dictionary<string, Device> devices = new Dictionary<string, Device>();

    private ClientWebSocket socket;
    private Uri uri;
    private CancellationToken cancellationToken = new CancellationTokenSource().Token;
    private bool aborted = false;

    private string name;
    private string host;
    private int port;

    public event ServerConnectedHandler ServerConnected;
    public event ServerDisconnectedHandler ServerDisconnected;
    public event DeviceAddedHandler DeviceAdded;
    public event DeviceRemovedHandler DeviceRemoved;

    public IReadOnlyList<Device> Devices {
      get {
        return devices.Values.Cast<Device>().ToList().AsReadOnly();
      }
    }

    public string Name {
      get {
        return name;
      }
    }

    public string Host {
      get {
        return host;
      }
    }

    public int Port {
      get {
        return port;
      }
    }

    public Client Client {
      get {
        return client;
      }
    }

    [DataContract]
    private class ServerMessage {
      [DataMember(Name = "defSwitchVector")]
      public SwitchProperty DefSwitchVector;
      [DataMember(Name = "defTextVector")]
      public TextProperty DefTextVector;
      [DataMember(Name = "defNumberVector")]
      public NumberProperty DefNumberVector;
      [DataMember(Name = "defLightVector")]
      public LightProperty DefLightVector;
      [DataMember(Name = "defBLOBVector")]
      public BLOBProperty DefBLOBVector;
      [DataMember(Name = "setSwitchVector")]
      public SwitchProperty SetSwitchVector;
      [DataMember(Name = "setTextVector")]
      public TextProperty SetTextVector;
      [DataMember(Name = "setNumberVector")]
      public NumberProperty SetNumberVector;
      [DataMember(Name = "setLightVector")]
      public LightProperty SetLightVector;
      [DataMember(Name = "setBLOBVector")]
      public BLOBProperty SetBLOBVector;
      [DataMember(Name = "deleteProperty")]
      public Property DeleteProperty;
    }

    public Server(string name, string host, int port, Client client) {
      this.name = name;
      this.host = host;
      this.port = port;
      this.client = client;
      uri = new Uri("ws://" + host + ":" + port);
      socket = new ClientWebSocket();
      socket.Options.KeepAliveInterval = new TimeSpan(365, 0, 0, 0, 0);
      new Thread(new ThreadStart(ConnectAsync)).Start();
    }

    public void Close() {
      aborted = true;
      socket.Abort();
    }

    public Device GetDevice(string name) {
      Device device = null;
      if (devices.TryGetValue(name, out device))
        return device;
      return null;
    }

    private Device GetOrAddDevice(string name) {
      Device device = null;
      if (!devices.TryGetValue(name, out device)) {
        device = new Device(name, this);
        devices.Add(device.Name, device);
        DeviceAdded?.Invoke(device);
      }
      return device;
    }

    private void DefProperty(Property property) {
      GetOrAddDevice(property.DeviceName).Add(property);
    }

    private void SetProperty(Property property) {
      GetOrAddDevice(property.DeviceName).Update(property);
    }

    private void DeleteProperty(Property property) {
      Device device = GetOrAddDevice(property.DeviceName);
      device.Delete(property);
      if (device.Properties.Count == 0) {
        devices.Remove(device.Name);
        DeviceRemoved?.Invoke(device);
      }
    }

    public void SendMessage(string message) {
      Log("Send:" + message);
      Task task = socket.SendAsync(new ArraySegment<byte>(Encoding.UTF8.GetBytes(message)), WebSocketMessageType.Text, true, cancellationToken);
      task.Wait();
    }

    private async void ConnectAsync() {
      var buffer = new byte[16 * 1024];
      try {
        await socket.ConnectAsync(uri, cancellationToken);
        OnConnect(this);
        while (socket.State == WebSocketState.Open) {
          var stringResult = new StringBuilder();
          WebSocketReceiveResult result;
          do {
            result = await socket.ReceiveAsync(new ArraySegment<byte>(buffer), cancellationToken);
            if (result.MessageType == WebSocketMessageType.Close) {
              await socket.CloseAsync(WebSocketCloseStatus.NormalClosure, string.Empty, CancellationToken.None);
              OnDisconnect(this);
            } else {
              var str = Encoding.UTF8.GetString(buffer, 0, result.Count);
              stringResult.Append(str);
            }
          } while (!result.EndOfMessage);
          OnMessage(stringResult.ToString(), this);
        }
      } catch (Exception exception) {
        if (aborted)
          Log("Socket aborted");
        else {
          Log(exception.Message);
          Log(exception.StackTrace);
          OnDisconnect(this);
        }
      } finally {
        socket.Dispose();
      }
    }

    private void OnMessage(string message, Server server) {
      Log("Received:" + message);
      DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(ServerMessage));
      MemoryStream stream = new MemoryStream(System.Text.ASCIIEncoding.ASCII.GetBytes(message));
      ServerMessage serverMessage = (ServerMessage)serializer.ReadObject(stream);
      client.Mutex.WaitOne();
      if (serverMessage.DefSwitchVector != null) {
        DefProperty(serverMessage.DefSwitchVector);
      } else if (serverMessage.DefTextVector != null) {
        DefProperty(serverMessage.DefTextVector);
      } else if (serverMessage.DefNumberVector != null) {
        DefProperty(serverMessage.DefNumberVector);
      } else if (serverMessage.DefLightVector != null) {
        DefProperty(serverMessage.DefLightVector);
      } else if (serverMessage.DefBLOBVector != null) {
        DefProperty(serverMessage.DefBLOBVector);
      } else if (serverMessage.SetSwitchVector != null) {
        SetProperty(serverMessage.SetSwitchVector);
      } else if (serverMessage.SetTextVector != null) {
        SetProperty(serverMessage.SetTextVector);
      } else if (serverMessage.SetNumberVector != null) {
        SetProperty(serverMessage.SetNumberVector);
      } else if (serverMessage.SetLightVector != null) {
        SetProperty(serverMessage.SetLightVector);
      } else if (serverMessage.SetBLOBVector != null) {
        SetProperty(serverMessage.SetBLOBVector);
      } else if (serverMessage.DeleteProperty != null) {
        DeleteProperty(serverMessage.DeleteProperty);
      } else {
        Log("Failed to process message: " + message);
      }
      client.Mutex.ReleaseMutex();
    }

    private void OnConnect(Server server) {
      Log("Connected to " + Host + ":" + Port);
      client.Mutex.WaitOne();
      ServerConnected?.Invoke(server);
      server.SendMessage("{ 'getProperties': { 'version': 512 } }");
      client.Mutex.ReleaseMutex();
    }

    private void OnDisconnect(Server server) {
      client.Mutex.WaitOne();
      devices.Clear();
      ServerDisconnected?.Invoke(server);
      client.Mutex.ReleaseMutex();
      Log("Disconnected from " + Host + ":" + Port);
    }
  }

  public class Client: Indigo {
    private List<Server> servers = new List<Server>();

    public readonly Mutex Mutex = new Mutex();
    public event ServerAddedHandler ServerAdded;
    public event ServerRemovedHandler ServerRemoved;

    private DNSSDEventManager eventManager = new DNSSDEventManager();
    private DNSSDService service = new DNSSDService();
    private DNSSDService browser;
    private Dictionary<DNSSDService, string> resolvers = new Dictionary<DNSSDService, string>();

    public IReadOnlyList<Server> Servers {
      get {
        return servers.Cast<Server>().ToList().AsReadOnly();
      }
    }

    private void ServiceFound(DNSSDService browser, DNSSDFlags flags, uint ifIndex, string serviceName, string regType, string domain) {
      Log("Service " + serviceName + " found ");
      try {
        DNSSDService resolver = browser.Resolve(0, ifIndex, serviceName, regType, domain, eventManager);
        resolvers.Add(resolver, serviceName);
      } catch {
        Log("DNSSDService.Resolve() failed");
      }
    }

    private void ServiceLost(DNSSDService browser, DNSSDFlags flags, uint ifIndex, string serviceName, string regType, string domain) {
      Log("Service " + serviceName + " lost");
      Server server = null;
      Mutex.WaitOne();
      foreach (Server item in servers)
        if (item.Name == serviceName) {
          server = item;
          break;
        }
      if (server != null) {
        servers.Remove(server);
        ServerRemoved?.Invoke(server);
      }
      Mutex.ReleaseMutex();
    }

    private void ServiceResolved(DNSSDService resolver, DNSSDFlags flags, uint ifIndex, string fullName, string hostName, ushort port, TXTRecord txtRecord) {
      Log("Service " + hostName + ":" + port + " resolved");
      resolver.Stop();
      string serviceName = resolvers[resolver];
      Server server = new Server(serviceName, hostName, port, this);
      Mutex.WaitOne();
      servers.Add(server);
      resolvers.Remove(resolver);
      ServerAdded?.Invoke(server);
      Mutex.ReleaseMutex();
    }

    private void OperationFailed(DNSSDService service, DNSSDError error) {
      Log("Operation failed: " + error);
      service.Stop();
      resolvers.Remove(service);
    }

    public Client() {
      Log("Client started ");
      eventManager.ServiceFound += new _IDNSSDEvents_ServiceFoundEventHandler(ServiceFound);
      eventManager.ServiceLost += new _IDNSSDEvents_ServiceLostEventHandler(ServiceLost);
      eventManager.ServiceResolved += new _IDNSSDEvents_ServiceResolvedEventHandler(ServiceResolved);
      eventManager.OperationFailed += new _IDNSSDEvents_OperationFailedEventHandler(OperationFailed);
      try {
        browser = service.Browse(0, 0, "_indigo._tcp", null, eventManager);
      } catch {
        Log("DNSSDService.Browse() failed");
      }
    }

    public Device GetDevice(string name) {
      Device device = null;
      foreach (Server server in servers)
        if ((device = server.GetDevice(name)) != null)
          break;
      return device;
    }

    public void Dispose() {
      foreach (Server server in servers)
        server.Close();
      foreach (DNSSDService resolver in resolvers.Keys)
        resolver.Stop();
      resolvers.Clear();
      if (browser != null)
        browser.Stop();
      browser = null;
      if (service != null)
        service.Stop();
      Log("Client disposed");
    }
  }
}
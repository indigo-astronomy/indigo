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
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Net.WebSockets;
using System.Threading;
using System.Text;
using System.IO;
using System.Linq;
using Bonjour;

namespace INDIGO {

  public delegate void ServerAdded(Server server);
  public delegate void ServerRemoved(Server server);
  public delegate void DeviceAdded(Device device);
  public delegate void DeviceRemoved(Device device);
  public delegate void GroupAdded(Group group);
  public delegate void GroupRemoved(Group group);
  public delegate void PropertyAdded(Property property);
  public delegate void PropertyRemoved(Property property);

  [DataContract]
  public class Item {
    [DataMember]
    public string name;
    [DataMember]
    public string label;

    public override bool Equals(object other) {
      return name.Equals(((Item)other).name);
    }
  }

  [DataContract]
  public class SwitchItem : Item {
    [DataMember]
    public bool value;
  }

  [DataContract]
  public class TextItem : Item {
    [DataMember]
    public string value;
  }

  [DataContract]
  public class NumberItem : Item {
    [DataMember]
    public double min;
    [DataMember]
    public double max;
    [DataMember]
    public double step;
    [DataMember]
    public double value;
    [DataMember]
    public string format;
  }

  [DataContract]
  public class LightItem : Item {
    [DataMember]
    public string value;
  }

  [DataContract]
  public class BLOBItem : Item {
    [DataMember]
    public string value;
  }

  [DataContract]
  public class Property {
    [DataMember]
    public int version;
    [DataMember]
    public string device;
    [DataMember]
    public string name;
    [DataMember]
    public string group;
    [DataMember]
    public string label;
    [DataMember]
    public string perm;
    [DataMember]
    public string state;
    [DataMember]
    public string message;

    public virtual List<Item> items {
      get {
        return null;
      }
    }

    public override bool Equals(object other) {
      return name.Equals(((Property)other).name);
    }
  }

  [DataContract]
  public class SwitchProperty : Property {
    [DataMember]
    public string rule;
    [DataMember(Name = "items")]
    public List<SwitchItem> switches;

    override public List<Item> items {
      get {
       return switches.Cast<Item>().ToList();
      }
    }
  }

  [DataContract]
  public class TextProperty : Property {
    [DataMember(Name = "items")]
    public List<TextItem> texts;

    override public List<Item> items {
      get {
        return texts.Cast<Item>().ToList();
      }
    }
  }

  [DataContract]
  public class NumberProperty : Property {
    [DataMember(Name = "items")]
    public List<NumberItem> numbers;

    override public List<Item> items {
      get {
        return numbers.Cast<Item>().ToList();
      }
    }
  }

  [DataContract]
  public class LightProperty : Property {
    [DataMember(Name = "items")]
    public List<LightItem> lights;

    override public List<Item> items {
      get {
        return lights.Cast<Item>().ToList();
      }
    }
  }

  [DataContract]
  public class BLOBProperty : Property {
    [DataMember(Name = "items")]
    public List<BLOBItem> blobs;

    override public List<Item> items {
      get {
        return blobs.Cast<Item>().ToList();
      }
    }
  }

  public class Group {
    public string group;
    public List<Property> properties;
    public PropertyAdded propertyAdded;
    public PropertyRemoved propertyRemoved;

    public Group(string group) {
      this.group = group;
      properties = new List<Property>();
    }
  }

  public class Device {
    public string device;
    public Dictionary<string, Group> groups;
    public List<Property> properties;

    public GroupAdded groupAdded;
    public GroupRemoved groupRemoved;
    public PropertyAdded propertyAdded;
    public PropertyRemoved propertyRemoved;

    public Device() {
    }

    public Device(string device) {
      this.device = device;
      groups = new Dictionary<string, Group>();
      properties = new List<Property>();
    }
  }

  public class Server {

    public Dictionary<string, Device> devices;

    public string name;
    public string host;
    public int port;

    public DeviceAdded deviceAdded;
    public DeviceRemoved deviceRemoved;

    private const string GET_PROPERTIES = "{ 'getProperties': { 'version': 512 } }";

    private readonly ClientWebSocket ws;
    private readonly Uri uri;
    private readonly CancellationToken cancellationToken = new CancellationTokenSource().Token;

    [DataContract]
    private class ServerMessage {
      [DataMember]
      public SwitchProperty defSwitchVector;
      [DataMember]
      public TextProperty defTextVector;
      [DataMember]
      public NumberProperty defNumberVector;
      [DataMember]
      public LightProperty defLightVector;
      [DataMember]
      public BLOBProperty defBLOBVector;
      [DataMember]
      public SwitchProperty setSwitchVector;
      [DataMember]
      public TextProperty setTextVector;
      [DataMember]
      public NumberProperty setNumberVector;
      [DataMember]
      public LightProperty setLightVector;
      [DataMember]
      public BLOBProperty setBLOBVector;
      [DataMember]
      public Property deleteProperty;
    }

    public Server(string name, string host, int port) {
      this.name = name;
      this.host = host;
      this.port = port;
      devices = new Dictionary<string, Device>();
      uri = new Uri("ws://" + host + ":" + port);
      ws = new ClientWebSocket();
      ws.Options.KeepAliveInterval = new TimeSpan(365, 0, 0, 0, 0);
      new Thread(new ThreadStart(ConnectAsync)).Start();
    }

    private void defProperty(Property property) {
      Console.WriteLine("def(" + property + ") '" + property.device + "' '" + property.name + "'");
      Device device = null;
      if (!devices.TryGetValue(property.device, out device)) {
        device = new Device(property.device);
        devices.Add(device.device, device);
        if (deviceAdded != null)
          deviceAdded(device);
      }
      if (device.properties.Contains(property)) {
        Console.WriteLine("def(" + property + ") '" + property.device + "' '" + property.name + "' - duplicate def");
        return;
      }
      device.properties.Add(property);
      if (device.propertyAdded != null)
        device.propertyAdded(property);
      Group group = null;
      if (!device.groups.TryGetValue(property.group, out group)) {
        group = new Group(property.group);
        device.groups.Add(group.group, group);
        if (device.groupAdded != null)
          device.groupAdded(group);
      }
      group.properties.Add(property);
      if (group.propertyAdded != null)
        group.propertyAdded(property);
    }

    private void setProperty(Property property) {
      Console.WriteLine("set(" + property + ") '" + property.device + "' '" + property.name + "'");
      Device device = null;
      if (devices.TryGetValue(property.device, out device)) {
        foreach (Property prop in device.properties) {
          if (prop.Equals(property)) {
            prop.message = property.message;
            foreach (Item item in property.items) {
              foreach (Item itm in prop.items) {
                if (item.name == itm.name) {
                  if (item is SwitchItem)
                    ((SwitchItem)item).value = ((SwitchItem)itm).value;
                  else if (item is TextItem)
                    ((TextItem)item).value = ((TextItem)itm).value;
                  else if (item is NumberItem)
                    ((NumberItem)item).value = ((NumberItem)itm).value;
                  else if (item is LightItem)
                    ((LightItem)item).value = ((LightItem)itm).value;
                  else if (item is BLOBItem)
                    ((BLOBItem)item).value = ((BLOBItem)itm).value;
                  break;
                }
              }
            }
            return;
          }
        }
        Console.WriteLine("set(" + property + ") '" + property.device + "' '" + property.name + "' - not found!");
      }
    }

    private void deleteProperty(Property property) {
      Console.WriteLine("delete(" + property + ") '" + property.device + "' '" + property.name + "'");
      Device device = null;
      if (devices.TryGetValue(property.device, out device)) {
        if (property.name == null) {
          device.properties.Clear();
          device.groups.Clear();
        } else {
          foreach (Property prop in device.properties) {
            if (prop.Equals(property)) {
              device.properties.Remove(prop);
              if (device.propertyRemoved != null)
                device.propertyRemoved(prop);
              Group group = null;
              if (device.groups.TryGetValue(prop.group, out group)) {
                group.properties.Remove(prop);
                if (group.propertyRemoved != null)
                  group.propertyRemoved(prop);
                if (group.properties.Count == 0) {
                  device.groups.Remove(group.group);
                  if (device.groupRemoved != null)
                    device.groupRemoved(group);                
                }
              }
              break;
            }
          }
        }
        if (device.properties.Count == 0) {
          devices.Remove(device.device);
          if (deviceRemoved != null)
            deviceRemoved(device);
        }
      }
    }

    private async void SendMessageAsync(string message) {
      await ws.SendAsync(new ArraySegment<byte>(Encoding.UTF8.GetBytes(message)), WebSocketMessageType.Text, true, cancellationToken);
    }

    private async void ConnectAsync() {
      var buffer = new byte[16 * 1024];
      try {
        await ws.ConnectAsync(uri, cancellationToken);
        onConnect(this);
        while (ws.State == WebSocketState.Open) {
          var stringResult = new StringBuilder();
          WebSocketReceiveResult result;
          do {
            result = await ws.ReceiveAsync(new ArraySegment<byte>(buffer), cancellationToken);
            if (result.MessageType == WebSocketMessageType.Close) {
              await ws.CloseAsync(WebSocketCloseStatus.NormalClosure, string.Empty, CancellationToken.None);
              onDisconnect(this);
            } else {
              var str = Encoding.UTF8.GetString(buffer, 0, result.Count);
              stringResult.Append(str);
            }
          } while (!result.EndOfMessage);
          onMessage(stringResult.ToString(), this);
        }
      } catch (Exception exception) {
        Console.WriteLine(exception.StackTrace);
        onDisconnect(this);
      } finally {
        ws.Dispose();
      }
    }

    private void onMessage(string message, Server server) {
      DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(ServerMessage));
      MemoryStream stream = new MemoryStream(System.Text.ASCIIEncoding.ASCII.GetBytes(message));
      ServerMessage serverMessage = (ServerMessage)serializer.ReadObject(stream);
      if (serverMessage.defSwitchVector != null) {
        defProperty(serverMessage.defSwitchVector);
      } else if (serverMessage.defTextVector != null) {
        defProperty(serverMessage.defTextVector);
      } else if (serverMessage.defNumberVector != null) {
        defProperty(serverMessage.defNumberVector);
      } else if (serverMessage.defLightVector != null) {
        defProperty(serverMessage.defLightVector);
      } else if (serverMessage.defBLOBVector != null) {
        defProperty(serverMessage.defBLOBVector);
      } else if (serverMessage.setSwitchVector != null) {
        setProperty(serverMessage.setSwitchVector);
      } else if (serverMessage.setTextVector != null) {
        setProperty(serverMessage.setTextVector);
      } else if (serverMessage.setNumberVector != null) {
        setProperty(serverMessage.setNumberVector);
      } else if (serverMessage.setLightVector != null) {
        setProperty(serverMessage.setLightVector);
      } else if (serverMessage.setBLOBVector != null) {
        setProperty(serverMessage.setBLOBVector);
      } else if (serverMessage.deleteProperty != null) {
        deleteProperty(serverMessage.deleteProperty);
      } else {
        Console.WriteLine("Failed to process message: " + message);
      }
    }

    private void onConnect(Server server) {
      Console.WriteLine("Connected to " + host + ":" + port);
      server.SendMessageAsync(GET_PROPERTIES);
    }

    private void onDisconnect(Server server) {
      Console.WriteLine("Disconnected from " + host + ":" + port);
    }
  }

  public class Client {
    public List<Server> servers = new List<Server>();

    public ServerAdded serverAdded;
    public ServerRemoved serverRemoved;

    private DNSSDEventManager eventManager = null;
    private Dictionary<DNSSDService, string> resolvers = new Dictionary<DNSSDService, string>();

    private void ServiceFound(DNSSDService browser, DNSSDFlags flags, uint ifIndex, string serviceName, string regType, string domain) {
      Console.WriteLine("Service " + serviceName + " found");
      try {
        DNSSDService resolver = browser.Resolve(0, ifIndex, serviceName, regType, domain, eventManager);
        resolvers.Add(resolver, serviceName);
      } catch {
        Console.WriteLine("DNSSDService.Resolve() failed");
      }
    }

    private void ServiceLost(DNSSDService browser, DNSSDFlags flags, uint ifIndex, string serviceName, string regType, string domain) {
      Console.WriteLine("Service " + serviceName + " found");
      Server server = null;
      foreach (Server item in servers)
        if (item.name == serviceName) {
          server = item;
          break;
        }
      if (server != null) {
        servers.Remove(server);
        serverRemoved(server);
      }
    }

    private void ServiceResolved(DNSSDService resolver, DNSSDFlags flags, uint ifIndex, string fullName, string hostName, ushort port, TXTRecord txtRecord) {
      Console.WriteLine("Service " + hostName + ":" + port + " resolved");
      resolver.Stop();
      string serviceName = resolvers[resolver];
      Server server = new Server(serviceName, hostName, port);
      servers.Add(server);
      resolvers.Remove(resolver);
      serverAdded(server);
    }

    private void OperationFailed(DNSSDService service, DNSSDError error) {
      Console.WriteLine("Operation failed: " + error);
      service.Stop();
      resolvers.Remove(service);
    }

    public Client() {
      eventManager = new DNSSDEventManager();
      eventManager.ServiceFound += new _IDNSSDEvents_ServiceFoundEventHandler(ServiceFound);
      eventManager.ServiceLost += new _IDNSSDEvents_ServiceLostEventHandler(ServiceLost);
      eventManager.ServiceResolved += new _IDNSSDEvents_ServiceResolvedEventHandler(ServiceResolved);
      eventManager.OperationFailed += new _IDNSSDEvents_OperationFailedEventHandler(OperationFailed);
      try {
        new DNSSDService().Browse(0, 0, "_indigo._tcp", null, eventManager);
      } catch {
        Console.WriteLine("DNSSDService.Browse() failed");
      }
    }
  }
}
# Design: locally stamped device or master token forwarded to remote servers

Ticket: ac9ea89e-aecd-4c94-8934-f3a2f9f2f3d0. Source tree: master at f9b1a34ed.
Basis: this design rests on reading the code. No experiment was run and nothing was built.

## Verdict

The leak is real in the code, but not in the form the review suspected, and not
in every deployment.

| # | Scenario | Leaks? | Reachable from binaries in this repository? |
|---|----------|--------|------------------------------------|
| L1 | `indigo_use_host_suffix == false`, a process that hosts local devices **and** is connected to remote servers: every change request for a *local* device is also sent to every remote server, carrying the local device token or the master token | yes | **no**. `indigo_server` keeps the default `true`. The only shipped binary that sets `false` is `indigo_prop_tool`, and it hosts no local devices. Third-party embedders cannot be checked from here. |
| L2 | `indigo_use_host_suffix == true` (indigo_server default), master token set (`-T` or `@` in the ACL), server chained with `-r host`, and a local agent, script or the Configuration Agent changes a property of a *remote* device (`X @ host`) that has no ACL entry of its own: the **local master token** goes to the remote server | yes | **yes**. The documentation describes this as client behaviour (see section 3). Whether it is intended for a server is open question Q1. |
| L3 | `indigo_use_host_suffix == true`, two remote servers whose adapter names share a prefix (`@ host` and `@ host:7625`, `@ indigo` and `@ indigosky`): a request for `X @ host:7625` also goes to server `host`, as device `X`, carrying the token meant for `host:7625` | yes, plus a misrouted change request | yes, but only with that naming pattern |
| L4 | `indigo_use_host_suffix == true`, request for a local device (config restore, `indigo_change_*_property` helpers) | no | n/a |
| L5 | A request from a network client, forwarded through a chained server | not a leak: the token was supplied by the client itself and is passed through unchanged | n/a |

The JSON protocol is not involved. `indigo_json.c` only has a server-side parser and
device adapter. Remote servers are reached only through the XML client adapter.

## Recommendation

**Option A** (routing fix in `indigo_change_property`) closes L1 and L3. These are
plain defects: requests, with their tokens, reach servers they were never addressed
to. The fix is low risk, the same on all platforms, and keeps every intended token
flow working.

L2 is a policy question, not a routing bug. Implement **Option B** (egress policy
for the master token at the TCP adapter) only if the owner answers Q1 with "the
server master token must not leave the server". Otherwise, document L2 as accepted
behaviour.

**Proposed severity: low**, because:

- L1 is not reachable from any shipped binary.
- L3 needs a particular naming of two servers.
- The project's own documentation says access control is "not meant to be security
  features" (`indigo_docs/INDIGO_DEVICE_ACCESS_CONTROL_AND_LOCKING.md:12`).

Raise it to **medium** if Q1 is answered "must not leave": L2 then becomes a
reachable disclosure of the credential that overrides every lock and protection on
the leaking server, sent in plaintext.

## 1. Evidence

### Routing (`indigo_libs/indigo_bus.c`)

- `bool indigo_use_host_suffix = true;` (`indigo_bus.c:449`). In this repository it
  is set to `false` only by `indigo_tools/indigo_prop_tool.c:941` and by tests
  (`test_protocol_security.c:548`, `test_server_driver_stress.c:704`,
  `test_agent_config.c:1064`). `indigo_server` never changes it.
- `indigo_change_property` (`indigo_bus.c:1000-1022`) offers the request to a device
  if any of these holds (lines 1008-1011):
  1. the request's device name is empty. Every device, adapters included, gets it
     whatever the suffix mode;
  2. the device name is equal to the request's device name;
  3. `indigo_use_host_suffix` is true, the device name starts with `@`, and
     `strstr(property->device, device->name)`. This is a **substring** test, not a
     suffix test, which causes L3;
  4. `indigo_use_host_suffix` is false and the device name starts with `@`. Every
     adapter gets **every** request, which causes L1.
- The ACL check (`indigo_bus.c:1013`) runs against the adapter device itself. Its
  `access_token` is reset to 0 on attach and then set to
  `indigo_get_device_token(device->name)`, that is, to the ACL entry `@ host` if one
  exists (`indigo_bus.c:882, 895`). Normally it is 0, so the request passes.
- `indigo_enumerate_properties` (`indigo_bus.c:985-990`) and `indigo_enable_blob`
  (`indigo_bus.c:1031-1036`) use the same routing. They carry no tokens, but they
  are misrouted in the same way as L3.

### Remote servers and the adapter

- `indigo_connect_server()` starts `server_thread` (`indigo_client.c:462-482`). The
  thread creates `indigo_xml_client_adapter(server->name, ...)`, where the name is
  the short host name plus `:port` when the port is not 7624
  (`indigo_service_name`, `indigo_client.c:445-460`), and attaches it to the bus.
- The adapter device is named `"@ %s"`. `is_remote` is true for TCP and false for
  the pipes of an `-i` subprocess (`indigo_client_xml.c:44-45`).
- In `indigo_server`, chaining happens only through `-r host[:port]`
  (`indigo_server.c:1751-1762`, which also sets `indigo_reshare_remote_devices`).
  `-i` driver subprocesses use the same kind of adapter, connected through pipes on
  the same host (`indigo_server.c:1764-1769`, Linux and macOS only). No agent and no
  service-discovery path calls `indigo_connect_server`.
- With the suffix on, incoming device names get `" @ host"` appended
  (`indigo_xml.c:590-595` and the matching handlers). So remote devices are visible
  on the local bus as `X @ host`.
- `indigo_xml_client_parser_change_property` (`indigo_xml.c:1984-2056`):
  - It does **not** check that the device belongs to that remote server. It
    forwards whatever the bus hands it.
  - With the suffix on, it strips the name at the last `@` (lines 1999-2006).
  - It writes `token='%llx'` whenever `property->access_token != 0` (lines
    2007-2009).

### Token stamping

- `indigo_get_device_or_master_token(name)` (`indigo_token.c:111-124`) returns the
  ACL entry for exactly `name`, otherwise the process-wide master token. There is a
  single process-wide ACL table and a single master token (`indigo_token.c:42-43`).
  A server uses it to verify incoming requests and to stamp its own outgoing
  requests.
- Places that stamp with the master token as fallback:
  - `config_restore_handler` (`indigo_driver.c:655`);
  - the `indigo_change_*_property` helpers (`indigo_bus.c:2328-2455`);
  - the agent helpers (`indigo_agent.c:83, 101`);
  - the filter: device selection CONNECTION, cached-property forwarding and
    `indigo_filter_forward_change_property` (`indigo_filter.c:512, 529, 631, 1159`);
  - the Configuration Agent (`indigo_agent_config.c:197, 351`);
  - the scripting agent (`indigo_agent_scripting.c:392`).
- The filter handles remote devices and agents explicitly (`indigo_filter.c:568`).
  Agents in a chained server can therefore select `X @ host` and send it a
  CONNECTION request stamped with the local master token. This is L2.
- The MCP agent forwards only the token its caller supplies
  (`indigo_agent_mcp.c:991-1040`) and stamps nothing itself.
- A device that is not protected but is connected with a token becomes locked to
  that token (`indigo_driver.c:1604-1612`). In L2, the local master token therefore
  also becomes the lock token of the remote device.

## 2. The intended use: a token for a remote device

- The documentation (`INDIGO_DEVICE_ACCESS_CONTROL_AND_LOCKING.md:45-58, 102-121`)
  makes the client ACL a password store. The framework stamps each request with the
  device token, otherwise with the client master token. The client master token is
  explicitly meant to be sent to servers, to lock devices or to use the server
  master.
- `indigo_prop_tool -T token` depends on exactly this: `indigo_set_master_token`
  (`indigo_prop_tool.c:1019`) followed by the `indigo_change_*_property` helpers
  (`indigo_prop_tool.c:733-784`).
- Remote device tokens are registered under the name the local bus uses:
  - with the suffix on: `Dome @ indigosky`. A lookup for a local device can never
    return it, and a lookup for a remote device can never return a local device
    token;
  - with the suffix off: the bare remote name. It cannot be told apart from a local
    device of the same name.
- The documented ACL entry `@ servername` ("master token used for authentication on
  the specified server", doc line 104) is **not implemented** for outgoing requests.
  Nothing looks it up when stamping. Its only effect is that the adapter `@
  servername` itself becomes protected on the local bus (`indigo_bus.c:895, 1013`).
- How to tell a legitimate token from a leaked one:
  - **Device tokens:** legitimate if and only if the request name ends exactly with
    `" " + adapter name` (suffix on), or no local device of that name exists (suffix
    off). Option A enforces this through routing.
  - **Master token:** it cannot be told apart by value. A token equal to the local
    master token is either a deliberate client credential (prop_tool, Control Panel)
    or a server's own override credential (L2). The difference lies in the
    **process role**, not in the request. Two pieces of information are reliable:
    - whether the request originated locally (`client == NULL` or
      `!client->is_remote`; network clients have `is_remote = true`,
      `indigo_driver_xml.c:62`, `indigo_driver_json.c:56`);
    - whether the process is a server enforcing its own ACL.
    Today nothing records the second.

## 3. Impact

- **Who gains what:**
  - The operator of a remote server, or anyone on the plaintext TCP path, learns the
    leaking server's master token (L2, and L1 in an embedding process).
  - Presented to the leaking server, the master token overrides every device
    protection and every exclusive lock (`indigo_bus.c:1013`). That means full
    control of mounts, domes and power boxes.
  - A leaked device token (L1, L3) grants control of that one device.
  - L3 also changes a device on the wrong server, if a device with the stripped
    name exists there. This is a functional defect regardless of tokens.
- **Preconditions:**
  - for L2: ACL enforcement switched on with a master token (opt-in, off by
    default), `-r` chaining, and a local agent, script or Configuration Agent
    addressing a remote device with no per-device ACL entry;
  - for L1: an embedding process with the suffix off, local devices, remote
    connections and tokens;
  - for L3: two chained servers whose names share a prefix (two ports on the same
    host is the simplest case).
- **Mitigating:**
  - The documentation explicitly disclaims access control as a security feature
    (doc line 12).
  - Every property value is already readable by every client.
  - With the suffix on, local device tokens never reach a remote server (L4).

## 4. Fix options

### Option A: exact routing in `indigo_change_property` (recommended)

- **Where:** `indigo_libs/indigo_bus.c` only, as a static route predicate used by
  `indigo_change_property`.
  - Suffix on: offer the request to adapter `@ host` only when the request name ends
    exactly with `" @ host"`. The name length must be greater than the suffix
    length, and the suffix must be preceded by a space.
  - Suffix off: before the loop, find out whether a device that is not an adapter
    (name not starting with `@`) has exactly this name. If one does, offer the
    request to no adapter.
  - Requests with an empty device name: keep the current behaviour.
- **Behaviour change:**
  - With the suffix on, requests stop going to a second server with a
    prefix-related name. Nothing intended depended on that.
  - With the suffix off, a request for a name that exists both locally and remotely
    now goes only to the local device. That was ambiguous before, and both were
    changed.
  - prop_tool is unaffected: it has no local devices.
  - Requests from network clients relayed through a chained server still reach the
    remote server with the client's own token.
- **Platforms:** plain C string comparison, no OS dependency, the same on Linux,
  macOS and Windows.
- **Still unprotected:**
  - L2 (master token to remote devices);
  - with the suffix off, a per-device token for a remote device still goes to
    *every* connected remote server, because adapters do not track which devices
    their server defines;
  - `indigo_enumerate_properties` and `indigo_enable_blob` stay misrouted for L3.
    They carry no tokens; aligning them with the same predicate is a one-line
    change, but it is a scope extension for the owner to approve (Q4).

### Option B: Option A plus a master-token egress policy at the TCP adapter (only if Q1 = "must not leave")

- **Where:**
  - `indigo_xml_client_parser_change_property` (`indigo_xml.c:2007`). On a TCP
    adapter (`device->is_remote`), for a locally originated request
    (`client == NULL || !client->is_remote`), when the master token is nonzero and
    `property->access_token == indigo_get_master_token()` and this process is a
    server enforcing its own ACL: send the token of the documented ACL entry
    `@ host` if one exists, otherwise send no token.
  - The "process is a server" condition needs one explicit signal, set by
    `indigo_server.c` in `server_main` when it loads `-T` or `-a`. This is the only
    addition to the public interface (`indigo_bus.h` or `indigo_token.h`).
  - `indigo_bus.c:895` must stop turning the entry `@ host` into a bus lock on the
    adapter device. Otherwise configuring that entry makes local requests with
    per-device tokens fail locally.
- **Behaviour change:**
  - A server chain that relies on a shared master token must add `<token> @ host`
    to the ACL of the upstream server.
  - Remote devices that agents connect are no longer locked to the upstream master
    token.
  - Clients (prop_tool, Control Panel) are unchanged, because they never set the
    server signal.
  - Network clients that send the upstream master token through a chain lose it at
    the hop. Arguably correct, but it is a change.
- **Platforms:** no OS dependency. The `-T` and `-a` parsing is common to all
  platforms.
- **Still unprotected:**
  - a device token for `X @ host` deliberately configured on the upstream server is
    still sent (as intended);
  - the plaintext transport;
  - embedding processes that do not set the signal.

### Rejected alternative

Stripping every locally stamped token at TCP adapters would break `indigo_prop_tool
-T`, device locking from clients, and the documented client ACL.

Moving the master-token fallback out of the twelve stamping sites into the adapter
would be a larger rework of the access-control design. It is out of scope.

## 5. Test plan (hardware-free, in-process)

Extend `indigo_test/unit/test_protocol_security.c`. It already runs the bus with
the suffix off, sets master and device tokens, and builds an XML client adapter at
line 97.

`test_protocol_xml.c:436-460` shows the capture technique: an adapter whose output
is a file handle (`indigo_uni_create_file` under `build/unit/`) records exactly what
would be sent to the remote server. Each case attaches such an adapter to the bus
(input handle unused, `version = INDIGO_VERSION_CURRENT`) next to a local fake
device, issues requests through the public API, and reads the capture file.

To also cover the shipping server mode, the cases switch `indigo_use_host_suffix`
locally and restore it afterwards. No second server process is needed:
`indigo_server` takes part only through `-r`, and the bus and adapter code under
test is the same in-process. The cases:

1. **L1 before/after (suffix off):** master token `0x5678`, device token `0x1234`
   for local device `Security`, adapter `@ Remote` attached. Calling
   `indigo_change_switch_property_1(NULL, "Security", ...)` must reach the local
   device. Before the fix the capture contains `device='Security'` with
   `token='1234'`. After the fix the capture contains nothing. Repeat without a
   device token: before the fix `token='5678'` appears.
2. **L3 before/after (suffix on):** adapters `@ host` and `@ host:7625` with separate
   captures, device token for `X @ host:7625`. A request to `X @ host:7625` must
   appear only in the `host:7625` capture, as `device='X'` with its token. Before the
   fix it also appears in the `host` capture.
3. **Regression, legitimate remote token (suffix on):** a device token for
   `X @ Remote` is forwarded as `device='X' token='...'`.
4. **Regression, suffix off with no local device of that name:** a request to
   `RemoteOnly` with master token set (the prop_tool path) is forwarded with
   `token='5678'`.
5. **Regression, network-client relay:** a request parsed through the server-side
   XML adapter with `client->is_remote = true` (as in
   `device_acl_requires_device_or_nonzero_master_token`) for `X @ Remote` is
   forwarded with the client's token unchanged.
6. **Only if Option B is chosen:**
   - with the server signal set and no `@ Remote` entry, a stamped request to
     `X @ Remote` has no token;
   - with `<T> @ Remote` configured, it carries `T`, and the adapter does not refuse
     the request locally;
   - with the signal cleared, the master token is forwarded (client behaviour).

How the cases run:

- The reproduction is case 1 and case 2 run against the current tree. They must
  fail before the change, and that result is recorded in a ticket note before the
  fix lands (bugfixes rule).
- The suite runs under `make -C indigo_test test-unit` (flow check `unit`).
- No new file is needed, so neither the Makefile nor the Xcode project changes. If
  the owner prefers a separate `test_remote_token_routing.c`, it must be added to
  `UNIT_TESTS` and to the Xcode project.

An optional end-to-end variant would extend
`indigo_test/integration/test_server_driver_stress.c`, which already spawns
`indigo_server` on 127.0.0.1. It would start the server with `-T` and `-r` pointing
to a fake XML listener in the test, then select a remote device through an agent.
It is heavier, depends on POSIX `fork`, and adds no coverage of the defect beyond
the unit cases. It is not proposed.

## 6. Open questions for the owner

- **Q1.** Must a server's master token (`indigo_server -T` or `@` in its ACL) ever be
  sent to servers chained with `-r`? Today it is sent whenever a local agent, script
  or the Configuration Agent addresses a remote device that has no per-device ACL
  entry. Is a shared master token across a trusted chain an intended configuration?
  The answer decides between Option A alone and Option B, and the severity.
- **Q2.** Should the documented ACL entry `@ servername` (per-server outgoing master
  token, doc line 104) be implemented, as Option B does, or removed from the
  documentation? Today it only protects the adapter device on the local bus.
- **Q3.** With the suffix off, is it acceptable that a name existing both locally and
  on a remote server now addresses only the local device (Option A)?
- **Q4.** Should the same exact-suffix routing also be applied to
  `indigo_enumerate_properties` and `indigo_enable_blob`? They carry no tokens but
  are misrouted in the same way. This is outside the ticket's scope.
- **Q5.** Do the external client applications (INDIGO Control Panel, Ain Imager and
  other embedders) set `indigo_use_host_suffix = false` while hosting local drivers
  and remote connections? If so, L1 is reachable there, and their maintainers should
  be told.

## 7. Not established

- No run confirmed any scenario. L1, L2 and L3 are derived from code reading only.
- Whether external client applications or third-party embedders reach L1 (Q5).
  Their sources are not in this repository.
- Whether any deployment relies on L2 (shared master tokens across chained servers).
  Only the owner or users can say.
- Behaviour of foreign INDI servers that receive a `token` attribute. It is ignored
  per the protocol, not verified.

# MCP agent

## Supported devices

INDIGO devices and agents with visible properties.

## Supported platforms

Portable C implementation. macOS builds and synthetic tests are verified; Linux and Windows execution remain unverified.

## License

INDIGO Astronomy open-source license. The existing private jsmn tokenizer is MIT licensed; its license is retained in `../../indigo_libs/externals/jsmn/jsmn.h`.

## Use

Load `indigo_server indigo_agent_mcp`, then connect `MCP Agent` with its standard `CONNECTION` property. The service is disabled by default. Its URL is `http://127.0.0.1:7624/mcp` on the default server port.

For Codex, the MCP server entry uses that URL. Claude Desktop local Code sessions can use the same HTTP URL through a project `.mcp.json` entry with `"type": "http"`. Desktop Chat local-process integrations require a compatible stdio-to-HTTP bridge; no bridge is provided here. Claude remote connectors originate from Anthropic's cloud and cannot directly reach a localhost server. See [Claude Desktop shared configuration](https://code.claude.com/docs/en/desktop#shared-configuration).

## Status

MCP `2025-11-25`: initialization, initialized notifications, ping and `list_devices`, `list_properties`, `get_property` and `set_property`. Discovery, reads and reversible text writes were verified with Codex CLI `0.166.0-alpha.5.1` and Claude Desktop `4.16336.0` in local Code mode, using the CCD and mount simulators. Client-side numeric/switch writes, BLOB downloads and Desktop Chat were not exercised.

## Notes

The agent returns JSON responses and closes each HTTP connection. It issues no session ID and provides no SSE stream. The endpoint rejects requests carrying `Origin` and has no authentication. Connecting the agent exposes all visible readable properties to clients that can reach the existing server listener, including readable token values. Write-only property schemas are discoverable, but their values are never cached or returned. `set_property` supports visible RW/WO text, number and switch properties, including server and agent controls. Light and BLOB writes are unsupported. Input is bounded to 64 KiB, 256 JSON tokens, depth 16 and eight admitted requests, with finite read deadlines.

`set_property` takes `device`, `property`, and an `items` object mapping exact item names to typed values. An optional `token` is a hexadecimal string of 1–16 digits; omitted means zero, with no inherited device/master token. A result with `dispatched: true` and `completion: "not_confirmed"` records a bus dispatch attempt, which does not prove token acceptance or device completion. Observe subsequent state using `get_property`, or `list_properties` for WO properties. Retrying a write may repeat the operation.

Results reflect the latest bus definitions and updates; reading does not poll hardware. Number values and targets remain distinct. BLOB items contain URLs, formats and sizes; payloads are neither copied nor encoded as base64. Local `/blob/` URLs are relative to the same INDIGO server. Results and cached text use dynamic allocation. Unload drains HTTP callbacks before detaching the internal bus client and releasing its cache/device.

## Testing

- 2026-11-04 15:51 3.0.0.0 mac arm64 Codex CLI 0.166.0-alpha.5.1 (simulators) 8/8 OK
- 2026-11-04 20:27 3.0.0.0 mac arm64 Claude Desktop Code 4.16336.0 (simulators) 9/9 OK
- 2026-11-05 09:02 3.0.0.1 mac arm64 simulator 30/30 OK

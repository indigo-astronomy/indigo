# Claude GUI interoperability observation

## Discovery/read phase

Observed by parent using native cua_repl, Claude Desktop4.16336.0, local Code tab, Opus5.5 Medium unchanged. Test session local_e9db76b9-b248-40f6-a487-c918e7f4f8bd, isolated temporaryworkspace /private/tmp/indigo-mcp-gui-smoke/workspace. Project configuration typehttp, URLhttp://127.0.0.1:56512/mcp. No global MCP config change.

Prompt required only indigo_smoke MCP tools: list_devices; list_properties(CCD Imager Simulator); get_property(CCD Imager Simulator,INFO); get_property(Mount Simulator,INFO). It forbade shell/files/browser/set_property, property changes and connecting devices. Only actual INDIGO CCD/mount software simulator drivers were loaded in a private testhome.

The native accessibility tree visibly showed Connectors1connected, followed by Claude finished the response. Expanding 'Loaded tools, used indigo smoke4times' revealed executed list_devices, list_properties, get_property and get_property entries. Expanded final tool entry displayed Input Device=Mount Simulator, Property=INFO and its actual JSON output: stateOk, permissionro, DEVICE_NAME=Mount Simulator, DEVICE_DRIVER=indigo_mount_simulator, DEVICE_VERSION=3.0.0.26, DEVICE_INTERFACE=1. The camera INFO result was DEVICE_NAME=CCD Imager Simulator, DEVICE_DRIVER=indigo_ccd_simulator, DEVICE_VERSION=3.0.0.46, DEVICE_INTERFACE=2.

list_devices returned13names:Server; CCD Imager Simulator; CCD Guider Simulator; CCD Bahtinov Mask Simulator; DSLR Simulator; CCD File Simulator; CCD Imager Simulator(wheel); CCD Imager Simulator(focuser); CCD Guider Simulator(guider); CCD Guider Simulator(AO); Mount Simulator; Mount Simulator(guider); MCP Agent. Parent records spaces before each parenthesized device suffix exactly as displayed in codex-calls.json, which agrees with this GUI result. list_properties returned7camera properties:INFO,SIMULATION,CONFIG,PROFILE_NAME,PROFILE,ADDITIONAL_INSTANCES,CONNECTION.

All4requested MCPcalls succeeded, with no set_property or other tool use shown. This is GUI Code verification, not Claude Chat/remote connector/stdio verification; no BLOB download or write action was exercised. Parent authored agent earlier, so GUI execution is manualverification rather than independentimplementation review.

## Reversible write phase
After the readphase, the actualsimulator fixture restarted on the same nowfreeport56512 in freshprivatehome (PID76091), and temporaryprojectconfiguration was restored. Parent sent a followup requiring exactly5sequential MCPcalls, with no otherpropertytarget/connection/exposure/motion/explicitCONFIG action: getPROFILE_NAME, setNAME_0 toMCP Claude verification, getreadback, setNAME_0 toexactoriginal, getrestoration. SameGUI/localCode/model selection.

Actual GUI showed Claude finished the response. The first tool group contained the initialget; expanding the following Used indigo smoke4times group exposed set_property/get_property/set_property/get_property. Parent expanded the intermediateget actualJSON result: CCD Imager Simulator.PROFILE_NAME, stateOk/permissionrw, NAME_0=MCP Claude verification; NAME_1..4=Profile #1..4. Parent then expanded the finalget actualJSON result: NAME_0=Profile #0 and NAME_1..4 unchanged. Bothsetresults shown as dispatchedtrue/completionnot_confirmed; completion was established by readback rather than those acknowledgements. All5calls succeeded. The native PROFILE_NAME handler automatically saves profile names inside the privatefixturehome; the client made no explicitCONFIGsavecall.

GUIcampaign total9/9 MCPcalls (4discovery/read plus5writecycle), including2textwritecalls and verifiedrestoration. Only textwriteinteroperability was exercised, not numeric/switchwrites, BLOBdownloads, Chat, physicalhardware or broadastronomy operations.

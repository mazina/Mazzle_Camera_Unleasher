# Mazzle Native Camera Bridge

Native BG3 camera, input, and WASD bridge using the normal Script Extender object and event machinery.

This independent source repository owns the native plugin, embedded Lua bootstrap, ABI mappings, and build entry point. Its experiment branch is **native-camera-wasd-bridge**.

## Build

Run build.cmd from this project. The verified build uses x64 MSVC 14.50.35717 and the immutable Detours dependency already installed under D:\BG3_Modding\SEDevelopment\dependencies.

Outputs go to C:\Users\mazzin\Desktop\BG3_Modding\_MyProjects\_Codex_Tools_Research\Mazzle_Native_Camera_Bridge\build (the canonical tools junction resolves to D:\BG3_Modding\Tools\Mazzle_Native_Camera_Bridge). The output consists of Camera_Unleasher.dll, symbols, and build intermediates. Run verify.cmd in that project's tools directory for the existing offline checks. Building does not deploy to BG3.

The build paths can be set with BRIDGE_TOOLS_DIR, BRIDGE_VCVARS, BRIDGE_DETOURS_ROOT, and BRIDGE_JSON_ROOT; the compiler version remains pinned. RapidJSON headers come from the same immutable SE dependency bundle as Detours. They are compiled into the DLL; no additional runtime file is required.

Normal builds compile with MAZZLE_BRIDGE_DIAGNOSTICS=0. To intentionally collect statistics for investigation, set BRIDGE_DIAGNOSTICS=1 before running build.cmd. That build compiles with MAZZLE_BRIDGE_DIAGNOSTICS=1 and writes to build-diagnostics instead of build, preserving the normal artifact. It exposes NativeStatistics, NativePayloadStatus, and NativePayloadValidationStatus under Ext.ClientCamera._Diagnostics. No statistics variables or updates are compiled into normal builds; functional startup/reset safety state remains. There is no user-facing setting or runtime toggle.

## Runtime

The native mod loader loads the DLL from BG3's bin\NativeMods. Only the DLL is required there. The build embeds editable source bridge.lua as a Windows RCDATA resource; Lua bootstrap changes require rebuilding the DLL. No external Lua bootstrap is read at runtime. Remove a legacy NativeMods/bridge.lua when upgrading. The game camera/WASD addresses are resolved once from the same instruction patterns used by the SE patch. There is no game-executable hash whitelist; mappings must identify unique executable targets with the supported entry instructions and valid data references. It resolves native SE functions directly from executable instruction anchors, PE function boundaries, native call relationships and class descriptors before installing any hooks. Bridge-owned ABI fields and descriptor/stride instructions retain exact witnesses. Missing, ambiguous or incompatible mappings refuse activation; no external symbol files are read.

Camera/input baseline build C0C0CF7D was live-validated on published Release C5E9041A (package73f6e7e, source af85fd2). All ten native events, serialization and borrowed-object lifetimes, input cancellation, pitch overrides, actual WASD movement, and Lua reset recovery passed, with no console errors or warnings. The current cursor-helper build 1F2A1433 is deployed; its complete offline checks pass against Release, published Devel791C11, and standalone DWrite3EDEFD76. The user confirmed cursor hiding and showing during movement and rotation. Those two other SE builds have not been live-tested with this DLL. The updater remains default Release (`{}`), without a pin. There is no SE DLL hash whitelist or address profile. The pattern resolver accepts recognized instruction shapes and requires compatible bridge ABI witnesses. Arbitrary compiler transformations are not assumed compatible.

Startup status is printed to the existing SE console, when enabled, with the
`[Camera Unleasher]` prefix. `STARTING` confirms the DLL is running; `LOADED`
confirms the client camera/input API initialized; `FAILED` includes the failure
reason. A keyboard-hook failure is reported separately as `WASD is unavailable`.
The file log stays in `%LOCALAPPDATA%\Larian Studios\Baldur's Gate 3\Script Extender\MazzleNativeCameraBridgeFull.log`, resolved using Windows' LocalAppData known folder.
If that directory or file cannot be opened, console reporting still works; no
alternate log destination is used. Console reporting does not require successful
SE address resolution and adds no camera/input-frame work. If the native mod
loader never loads the DLL, the bridge cannot print a status message.

Startup first looks for the normally named BG3ScriptExtender.dll. If absent,
it checks loaded DWrite.dll modules for SE's standard product name and core DLL
file description. Only these two DLL filenames are considered. The ordinary
updater and Windows DWrite library are excluded.
Multiple standalone SE candidates refuse activation. Resolution uses only the
loaded DLL: no symtab, PDB, debug directory, symbol server or build hash is used.
Relocations and SE-owned internal fields are variable operands. Native caller
relationships distinguish identical type-getter instruction templates, global
references must agree, and PE TLS metadata bounds the discovered context slot.
The bridge's own object, Lua, property-map and registration descriptor layouts
remain checked through native instruction witnesses and class descriptors.
All required targets must resolve uniquely; profiling helpers are an optional
pair. Unsupported instruction shapes or incompatible ABI refuse activation.

Successful camera/input dispatch performs no logging. A native dispatch failure
is logged at most once per client Lua VM, with further identical failures
suppressed until that VM is replaced or reset.

The game resolver passes offline against the current local Steam DirectX 11 and Vulkan executables, which have different function addresses. The revised DLL also passes live startup on Steam DirectX 11: all eight game targets resolve, camera hooks and the keyboard gate install, all ten native events are present, and Camera Unleashed becomes ready. The user also confirmed activation on the affected player's different executable; that player's renderer and store were not independently established.

## Lua interface

See [NATIVE_BRIDGE_API.md](NATIVE_BRIDGE_API.md). All ten camera/input event payloads and their borrowed camera/input/value objects are native SE objects. The public camera namespace is Ext.ClientCamera: its events use standard :Subscribe/:Unsubscribe methods, and GetCameraDefinition returns the existing native CameraGlobalSwitches type. WASD functions live only in Ext.Input.

Camera Unleashed requests movement binding setup once at gameplay initialization. This uses native WASD's policy: copy the four Camera movement bindings into CharacterMove bindings, including the controller Y-axis inversion, then reload only when the file changes. Existing independent CharacterMove direction bindings are replaced by the corresponding camera bindings; camera bindings and all unrelated preferences are preserved. Leaving the keybinding menu repeats setup while the client VM uses it. The actual game-selected player-one config path is captured, including custom profile roots. Cached binding snapshots let CU follow the configured movement/rotation keys without per-frame file access. Setup failure reports a warning and CU leaves movement inactive; explicit WASD commands retry setup after a fault. Missing setup mappings do not disable camera events.

Cursor hiding uses optional SDL exports through the existing client-update hook. `Ext.Input.SetCursorHidden` queues hide/show transitions, with shutdown releasing an outstanding request. It adds no game hooks, pattern targets, input subscriptions, or per-frame logging. While hiding, the existing update hook reapplies SDL visibility and redraws the current cursor; idle updates make no SDL calls. CU skips this feature when the helper is unavailable.

## Research and validation

Tests, probes, ABI research, build artifacts, validation reports, and runtime backups belong in the separate tools repositories. Current resolver and deployment evidence is D:\BG3_Modding\Tools\Mazzle_Native_Camera_Bridge\PatternSE-20261014\RESULTS.md. It records all target comparisons against three DLLs, 169 rejection checks, sanitizer checks, and Release live validation with the deployed artifact's hash. EmbeddedBootstrap-20261012 records resource byte equality and single-DLL packaging. Full native contract and prior Release/Devel resolver evidence remain in ClientCameraAPI-20261012 and RuntimeResolver-20261012T192902Z. Earlier native event proofs remain historical evidence in the camera tools repository.

Previous prototypes and source snapshots in the camera tools repository are historical research. The active implementation is owned by this repository.

The ClientCamera namespace and default-disabled diagnostics source changes are validated separately in the ClientCameraAPI-20261012 tools case. That historical case deployed a normal DLL and external Lua companion. EmbeddedBootstrap-20261012 validates the single-DLL replacement. All ten native payloads, namespace identity, input prevention, WASD movement, and reset recovery passed; all 52 consumer files remained unchanged. The earlier resolver runtime record remains historical two-build evidence.

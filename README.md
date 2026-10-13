# Mazzle Native Camera Bridge

Native BG3 camera, input, and WASD bridge using the normal Script Extender object and event machinery.

This independent source repository owns the native plugin, embedded Lua bootstrap, ABI mappings, and build entry point. Its experiment branch is **native-camera-wasd-bridge**.

## Build

Run build.cmd from this project. The verified build uses x64 MSVC 14.50.35717 and the immutable Detours dependency already installed under D:\BG3_Modding\SEDevelopment\dependencies.

Outputs go to C:\Users\mazzin\Desktop\BG3_Modding\_MyProjects\_Codex_Tools_Research\Mazzle_Native_Camera_Bridge\build (the canonical tools junction resolves to D:\BG3_Modding\Tools\Mazzle_Native_Camera_Bridge). The output consists of MazzleNativeCameraBridgeFull.dll, symbols, and build intermediates. Run verify.cmd in that project's tools directory for the existing offline checks. Building does not deploy to BG3.

The build paths can be set with BRIDGE_TOOLS_DIR, BRIDGE_VCVARS, and BRIDGE_DETOURS_ROOT; the compiler version remains pinned.

Normal builds compile with MAZZLE_BRIDGE_DIAGNOSTICS=0. To intentionally collect statistics for investigation, set BRIDGE_DIAGNOSTICS=1 before running build.cmd. That build compiles with MAZZLE_BRIDGE_DIAGNOSTICS=1 and writes to build-diagnostics instead of build, preserving the normal artifact. It exposes NativeStatistics, NativePayloadStatus, and NativePayloadValidationStatus under Ext.ClientCamera._Diagnostics. No statistics variables or updates are compiled into normal builds; functional startup/reset safety state remains. There is no user-facing setting or runtime toggle.

## Runtime

The native mod loader loads the DLL from BG3's bin\NativeMods. Only the DLL is required there. The build embeds editable source bridge.lua as a Windows RCDATA resource; Lua bootstrap changes require rebuilding the DLL. No external Lua bootstrap is read at runtime. Remove a legacy NativeMods/bridge.lua when upgrading. The plugin retains the game executable hash guard. It resolves native SE functions from the matching BG3ScriptExtender.symtab beside the loaded SE DLL, then verifies supported instruction/layout forms and helper/global relationships before installing any hooks. Missing, ambiguous, stale, or unsupported mappings refuse activation. The normal updater cache ships both files together.

The active normal build is 34678B65 with B90BB9 Lua bootstrap embedded, live-validated on published Release C5E9041A (package73f6e7e, source af85fd2). The updater remains default Release (`{}`), without a pin. This build also passes offline resolver checks against published Devel791C11 (package8d13e4cd, source45f32c1); its namespace changes have not been live-tested on Devel. The previous unchanged 00F341 build was live-tested on both published builds. There is no SE DLL hash whitelist or address profile. Recognized native instruction forms remain conservative: another compiler/body or unsupported ABI is rejected pending review; this is not universal compatibility.

## Lua interface

See [NATIVE_BRIDGE_API.md](NATIVE_BRIDGE_API.md). All ten camera/input event payloads and their borrowed camera/input/value objects are native SE objects. The public camera namespace is Ext.ClientCamera: its events use standard :Subscribe/:Unsubscribe methods, and GetCameraDefinition returns the existing native CameraGlobalSwitches type. WASD functions live only in Ext.Input.

## Research and validation

Tests, probes, ABI research, build artifacts, validation reports, and runtime backups belong in the separate tools repositories. The current packaging evidence is D:\BG3_Modding\Tools\Mazzle_Native_Camera_Bridge\EmbeddedBootstrap-20261012\RESULTS.md. It records resource byte equality, single-DLL startup, native event delivery/lifetimes, reset recovery and immutable rollback artifacts. Full native contract and prior Release/Devel resolver evidence remain in ClientCameraAPI-20261012 and RuntimeResolver-20261012T192902Z. Earlier native event proofs remain historical evidence in the camera tools repository.

Previous prototypes and source snapshots in the camera tools repository are historical research. The active implementation is owned by this repository.

The ClientCamera namespace and default-disabled diagnostics source changes are validated separately in the ClientCameraAPI-20261012 tools case. That historical case deployed a normal DLL and external Lua companion. EmbeddedBootstrap-20261012 validates the single-DLL replacement. All ten native payloads, namespace identity, input prevention, WASD movement, and reset recovery passed; all 52 consumer files remained unchanged. The earlier resolver runtime record remains historical two-build evidence.

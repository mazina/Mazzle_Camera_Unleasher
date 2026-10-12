# Mazzle Native Camera Bridge

Native BG3 camera, input, and WASD bridge using the normal Script Extender object and event machinery.

This independent source repository owns the native plugin, Lua registration companion, ABI mappings, and build entry point. Its experiment branch is **native-camera-wasd-bridge**.

## Build

Run build.cmd from this project. The verified build uses x64 MSVC 14.50.35717 and the immutable Detours dependency already installed under D:\BG3_Modding\SEDevelopment\dependencies.

Outputs go to C:\Users\mazzin\Desktop\BG3_Modding\_MyProjects\_Codex_Tools_Research\Mazzle_Native_Camera_Bridge\build (the canonical tools junction resolves to D:\BG3_Modding\Tools\Mazzle_Native_Camera_Bridge). The output consists of MazzleNativeCameraBridgeFull.dll, bridge.lua, symbols, and build intermediates. Run verify.cmd in that project's tools directory for the existing offline checks. Building does not deploy to BG3.

The build paths can be set with BRIDGE_TOOLS_DIR, BRIDGE_VCVARS, and BRIDGE_DETOURS_ROOT; the compiler version remains pinned.

## Runtime

The native mod loader loads the DLL from BG3's bin\NativeMods. Place bridge.lua beside the DLL. The plugin verifies the exact SE DLL and game executable hashes before installing hooks.

The tested active runtime is candidate 085CB77 with official Release v33 digest 73f6e7e (SE DLL C5E9041A, source af85fd2). The updater configuration is `{}` and uses the default Release channel; no version or digest pin is active. The bridge supports this exact verified DLL and game executable. A future updater release with a different DLL hash will safely refuse hooks until its ABI is mapped and tested.

## Lua interface

See [NATIVE_BRIDGE_API.md](NATIVE_BRIDGE_API.md). All ten camera/input event payloads and their borrowed camera/input/value objects are native SE objects. The explicit camera-definition getter returns the existing native CameraGlobalSwitches type.

## Research and validation

Tests, probes, ABI research, build artifacts, validation reports, and runtime backups belong in the separate tools repositories. The current Release evidence is D:\BG3_Modding\Tools\Mazzle_Native_Camera_Bridge\ReleaseRetarget-20261012T225424Z\RESULTS.md. It includes exact published-package provenance, all mapped native ABI entries, offline negative tests, all ten native event contracts, input prevention, WASD control, reset recovery, and immutable rollback artifacts. Earlier native event proofs remain historical evidence in the camera tools repository.

Previous prototypes and source snapshots in the camera tools repository are historical research. The active implementation is owned by this repository.

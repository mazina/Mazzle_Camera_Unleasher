# Mazzle Camera Unleasher

This branch targets Script Extender v33.

The plugin exposes native BG3 camera events and keyboard movement controls. See [the Lua API reference](NATIVE_BRIDGE_API.md) for the public interface.

Install `Mazzle_Camera_Unleasher.dll` in the game's `bin/NativeMods` folder. The DLL includes its Lua bootstrap.

To build from source, run `build.cmd` with x64 MSVC 14.50.35717, Detours, RapidJSON, and SDL2 headers. Set `BRIDGE_VCVARS`, `BRIDGE_DETOURS_ROOT`, `BRIDGE_JSON_ROOT`, and `BRIDGE_SDL_ROOT` for your toolchain. Set `BRIDGE_TOOLS_DIR` to the directory where the script should write its `build` folder.

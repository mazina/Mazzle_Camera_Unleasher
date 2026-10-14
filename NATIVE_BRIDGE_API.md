# Native bridge Lua interface

The plugin registers these client events through the existing SE event manager. Each payload is a distinct native SE object inheriting lua::EventBase.

| Events | Payload fields |
|---|---|
| CameraUpdating, CameraUpdated | Entity, Camera, DeltaTime |
| CameraPitchUpdating, CameraPitchUpdated, CameraZoomUpdated | Camera, DeltaTime |
| CameraPitchCalculated | Camera, IgnoreAvoidance, SelectMode, OriginalPitch, Pitch |
| CameraInput | Camera, Input, Result |
| CameraInputHandled | Camera, Input, Result |
| CameraZoomResetting, CameraZoomReset | Camera |

Event context fields are read-only. CameraPitchCalculated.Pitch is a writable float, and CameraInput.Result is a writable uint16. CameraInputHandled.Result is read-only. The native camera and input objects referenced by read-only fields retain their usual writable properties.

Use Ext.ClientCamera.<EventName>:Subscribe(callback, options) and :Unsubscribe(id). These reference the same native SE event objects registered in the normal event manager; Ext.Events continues to resolve them. SE provides priority, one-shot subscriptions, propagation stopping, custom properties, reflection, validation, and standard object serialization. Calling PreventAction() on CameraInput suppresses native handling and the handled event, returning the event's Result. Finite successful pitch overrides replace the engine result; NaN and infinity retain the original pitch.

event.Camera is ecl::GameCameraBehavior. event.Input is input::InputEvent; its OldValue and NewValue are input::InputValue. All references borrowed from a callback expire when its native callback lifetime ends. Store copied scalar values for later use.

Get the camera definition with:

    local definition = Ext.ClientCamera.GetCameraDefinition(event.Camera)

This helper uses SE's normal typed object extraction and returns native CameraGlobalSwitches with the current SE lifetime. Wrong types and expired cameras are rejected through the normal SE checks. Existing camera-mod getter adapters call this helper.

Keyboard movement uses Ext.Input.SetKeyboardMovementOverride(enabled) and Ext.Input.GetKeyboardMovementOverride(). The setter requires a boolean and returns whether the native gate is available. Native movement eligibility and character movement bindings still apply; VM shutdown resets the override.

The bridge also supplies these setup/cache helpers under Ext.Input:

| Function | Result |
|---|---|
| SyncKeyboardMovementBindings() | Boolean success. Synchronizes the active player-one config and reloads it if changed. Call once when gameplay is ready, or explicitly to retry a reported failure. |
| GetKeyboardMovementBindings() | Ordinary Lua table of binding string arrays for CameraForward/Backward/Left/Right, CameraRotateLeft/Right, and CameraToggleMouseRotate; nil when no successful snapshot exists. |
| GetKeyboardMovementBindingVersion() | Integer revision. Cheap to check each frame; retrieve and parse a new snapshot only when it changes. |

Synchronization follows native WASD's mapping policy: CameraForward/Backward/Left/Right replace the corresponding CharacterMove arrays, with the controller left-stick Y direction flipped. Missing camera direction entries use native WASD defaults; explicitly empty arrays remain empty. This intentionally replaces independent CharacterMove direction assignments. Camera bindings, modifiers, alternative keys, and unrelated input preferences remain unchanged. The file is replaced through a temporary file in the same directory, and unchanged configurations are not rewritten or reloaded.

The path comes from the game's input-config reload call rather than an assumed drive, user, or profile name. Once synchronization succeeds, leaving the keybinding menu refreshes the file and snapshot. Shutdown stops automatic synchronization and clears the movement override again after Lua shutdown subscribers return. Synchronization does no per-frame work; the snapshot is an ordinary copied table with no borrowed engine references. A failed remap synchronization invalidates the snapshot and disables the native override. CU reports the failure once and retries only on an explicit movement command or a new VM initialization.

These helpers add setup capabilities to the bridge; the gate-only Set/GetKeyboardMovementOverride semantics and SE source are unchanged. Camera Unleashed retains its existing patched-SE path when the setup helpers are absent.

Native serialization follows SE rules: Ext.Types.Serialize(event) omits pointer fields. Direct JSON serialization requires the normal options for userdata/internal types, such as IterateUserdata=true and StringifyInternalTypes=true; stock default JSON options reject these native objects.

Before the startup-reporting and game-address resolver changes, normal build 34678B65 was live-tested on published Release C5E9041A. The current SE resolver passes offline against both that Release and Devel791C11. The preceding 00F341 build has same-DLL live proof on both published builds. It resolves their matching shipped sibling symtabs and accepts only verified native instruction/layout forms, with bounded helper/global recognition. No SE version/hash whitelist or address profile is used; unsupported or ambiguous forms refuse hooks before activation. Game camera/WASD addresses are resolved from shared SE-patch instruction patterns, with unique-target and native instruction/reference checks; no game-executable hash whitelist is used. Final updater configuration is default Release (`{}`), without a pin. Exact patch API symmetry, a direct CameraDefinition property, and underlying ECS ClientCamera system reflection remain outside this implementation. The public Ext.ClientCamera Lua namespace is available.

Startup status uses the `[Camera Unleasher]` console prefix: `LOADED` means the
client camera/input bindings and embedded Lua registration completed; `FAILED`
includes the reason activation stopped. Console output uses Windows' existing
console handle, so resolver failures can be reported before SE functions are
available. The log uses the current user's Script Extender directory under
LocalAppData, with no alternate destination. These reports are startup-only;
the event and input paths are unchanged.

Camera events and GetCameraDefinition are published under Ext.ClientCamera. Keyboard movement functions are published only under Ext.Input. Ext._Internal.MazzleNativeBridge is not created.

Normal builds contain no statistics counters, counter updates, or diagnostic Lua functions. An intentional maintainer diagnostic build exposes Ext.ClientCamera._Diagnostics; there is no gameplay setting or runtime switch. See README.md for the build flag.

Deploy only Camera_Unleasher.dll in NativeMods. The Lua registration bootstrap is embedded in the DLL; bridge.lua remains editable source and requires a rebuild when changed. No separate Lua file is needed at runtime.

SE discovery prefers BG3ScriptExtender.dll. When it is absent, loaded DWrite.dll
modules are checked for the standard SE product name and core DLL description.
The updater and Windows DWrite do not qualify. The matching symbol table must
use the selected DLL's basename (DWrite.symtab for standalone SE), beside that
DLL. This discovery does not relax native instruction or ABI validation.

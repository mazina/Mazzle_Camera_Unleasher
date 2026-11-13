# Native bridge Lua interface

When the loaded Script Extender provides native camera events, the bridge skips initialization before installing hooks or Lua bindings. Consumers should prefer the standard `Ext.Events` camera events and `GameCameraBehavior.CameraDefinition`, falling back to `Ext.ClientCamera` and `GetCameraDefinition` on supported SE builds without those APIs. Backend selection can be cached for the lifetime of the Lua VM; borrowed camera/definition proxies must still be used only within their valid callbacks.


`NativeMods/Camera_Unleasher.dll` is the complete plugin. Its exported `Init()` registers with `SEUpdaterNotifyWhenLoaded` so camera types can be installed before SE type registration. With older updaters, initialization runs synchronously inside Native Mod Loader's `Init()` callback, after DLL loading and before game CRT initialization. It must not be postponed until `SDL_Init`; that is too late for camera type registration. The native-SE bypass remains unchanged. No shared coordinator, separate loader/payload, or Audio2Face dependency is required. The bridge requires SE native API 1.0 for Lua lifecycle callbacks and VM entry/exit. Bindings are recreated on client `Loading`; shutdown invalidates dispatch and restores input/cursor state. Replace the process-lifetime DLL only with BG3 closed.


The plugin registers these client events through the existing SE event manager. Each payload is a distinct native SE object inheriting lua::EventBase.

| Events | Payload fields |
|---|---|
| CameraCommitting | Entity, Eye, Target |
| CameraUpdating, CameraUpdated | Entity, Camera, DeltaTime |
| CameraPitchUpdating, CameraPitchUpdated, CameraZoomUpdated | Camera, DeltaTime |
| CameraPitchCalculated | Camera, IgnoreAvoidance, SelectMode, OriginalPitch, Pitch |
| CameraInput | Camera, Input, Result |
| CameraInputHandled | Camera, Input, Result |
| CameraZoomResetting, CameraZoomReset | Camera |

Event context fields are read-only. CameraCommitting.Eye and CameraCommitting.Target are writable vectors. CameraPitchCalculated.Pitch is a writable float, and CameraInput.Result is a writable uint16. CameraInputHandled.Result is read-only. The native camera and input objects referenced by read-only fields retain their usual writable properties.

Use Ext.ClientCamera.<EventName>:Subscribe(callback, options) and :Unsubscribe(id). These reference the same native SE event objects registered in the normal event manager; Ext.Events continues to resolve them. SE provides priority, one-shot subscriptions, propagation stopping, custom properties, reflection, validation, and standard object serialization. Calling PreventAction() on CameraInput suppresses native handling and the handled event, returning the event's Result. Finite successful pitch overrides replace the engine result; NaN and infinity retain the original pitch.

event.Camera is ecl::GameCameraBehavior. event.Input is input::InputEvent; its OldValue and NewValue are input::InputValue. All references borrowed from a callback expire when its native callback lifetime ends. Store copied scalar values for later use.

`CameraCommitting` runs immediately before the native camera transform is submitted. For gameplay cameras this is after obstruction/avoidance placement, inside `CameraUpdating`/`CameraUpdated`. It also applies to other cameras using this commit routine; it is not a once-per-frame notification.

The payload has read-only `Entity` and writable `Eye` and `Target` world-space `vec3` values. Assign `{x, y, z}` tables to these fields to replace the submitted position and look-at target. Changes affect this submission only; they do not overwrite the gameplay camera's target or smoothing state. No `Camera` field is supplied because this routine also accepts cameras without `GameCameraBehavior`. Filter by `Entity` when an override should affect a particular camera.

The event cannot prevent the native commit. Failed dispatch, nonfinite coordinates, and zero-length or nonfinite eye-to-target squared distance retain both original vectors. Event objects expire after the callback; copy values that must be retained.

Get the camera definition with:

    local definition = Ext.ClientCamera.GetCameraDefinition(event.Camera)

This helper uses SE's normal typed object extraction and returns native CameraGlobalSwitches with the current SE lifetime. Wrong types and expired cameras are rejected through the normal SE checks. Existing camera-mod getter adapters call this helper.

Keyboard movement uses Ext.Input.SetKeyboardMovementOverride(enabled) and Ext.Input.GetKeyboardMovementOverride(). The setter requires a boolean and returns whether the native gate is available. Native movement eligibility and character movement bindings still apply; VM shutdown resets the override.

`Ext.Input.SetCursorHidden(boolean)` requests forced cursor hiding. It returns whether the loaded SDL cursor exports are available. `true` keeps the cursor hidden until `false` ends that request and shows the cursor again. A `false` request while idle does nothing. This controls visibility only; BG3 still selects the cursor image. Lua VM shutdown ends any outstanding hide request after shutdown callbacks return.

The request is applied by the bridge's existing client-update hook. Hiding uses SDL's exported `SDL_ShowCursor` and `SDL_SetCursor` functions, without additional game hooks, mappings, or input subscriptions. Idle updates make no SDL calls; hidden updates reapply visibility without logging. Callers should send requests on transitions rather than every frame. This helper is bridge-specific and is not required by the camera or movement APIs.

`Ext.Input.SetRotationCursorEnabled(boolean)` arms native rotation cursor handling. It returns whether all required game call sites and SDL exports were installed successfully. Set `true` before rotation begins; the native rotation-start hook captures the window-relative cursor position. SDL relative mode confines and hides the mouse while retaining continuous motion. Native release restores the saved position before leaving relative mode, including BG3's virtual cursor. Repeated rotation-start notifications do not replace the saved position. This API is independent of `SetCursorHidden`; do not combine the two for rotation.

Set `false` when the owning feature stops, yields, or its setting is disabled. Requests and Lua VM shutdown are applied by the next main-thread SDL poll, with restoration data retained in native state. Focus loss also releases confinement and requires a new rotation-start notification before recapture. SDL hints are restored after an owned session. A relative mode already owned elsewhere is left untouched; a failed acquisition is not retried every frame. The return value reports API availability, not asynchronous SDL acquisition success.

Send policy changes on transitions. Position capture uses native rotation notifications; confinement and virtual-cursor restoration use the game's SDL polling path and do not call Lua. Idle hooks make no SDL calls, allocations, or log writes. Relative-mode changes, cursor warps, and synthetic refresh events occur only on transitions. The virtual cursor retains original WASD's six-pixel drag threshold before hover suppression.

The bridge also supplies this setup helper under Ext.Input:

| Function | Result |
|---|---|
| SyncKeyboardMovementBindings() | Boolean success. Synchronizes the active player-one config and reloads it if changed. Call once when gameplay is ready, or explicitly to retry a reported failure. |

Synchronization follows native WASD's mapping policy: CameraForward/Backward/Left/Right replace the corresponding CharacterMove arrays, with the controller left-stick Y direction flipped. Missing camera direction entries use native WASD defaults; explicitly empty arrays remain empty. This intentionally replaces independent CharacterMove direction assignments. Camera bindings, modifiers, alternative keys, and unrelated input preferences remain unchanged. The file is replaced through a temporary file in the same directory, and unchanged configurations are not rewritten or reloaded.

The path comes from the game's input-config reload call rather than an assumed drive, user, or profile name. Once synchronization succeeds, leaving the keybinding menu refreshes the mappings. Shutdown stops automatic synchronization and clears the movement override again after Lua shutdown subscribers return. Synchronization does no per-frame work. A failed remap synchronization disables the native override.

Native serialization follows SE rules: Ext.Types.Serialize(event) omits pointer fields. Direct JSON serialization requires the normal options for userdata/internal types, such as IterateUserdata=true and StringifyInternalTypes=true; stock default JSON options reject these native objects.

Camera/input baseline build C0C0CF7D was live-tested on published Release C5E9041A: all ten native events, serialization and lifetimes, input cancellation, pitch overrides, actual WASD movement, and Lua reset recovery passed. The user confirmed cursor hide/show behavior with build 1F2A1433. Startup-order build 927F19DF remains deployed and passes the complete offline checks against Release, Devel791C11 and standalone DWrite3EDEFD76. The same DLL passes live startup, all ten native event payloads, reflection, serialization, field writes, lifetimes, initialized WASD bindings, and Lua reset recovery on Release and standalone DWrite, with no console errors or warnings. Devel has offline resolver coverage only for this build. Subsequent enumeration/error-code and deferred-log fixes passed the native build and 12 isolated startup regression scenarios; they have not been deployed or live-tested. The resolver discovers native functions from instruction anchors and native call/data relations in the loaded DLL, with PE function boundaries, class descriptors and bridge ABI witnesses. No SE version/hash whitelist or address profile is used; unsupported or ambiguous forms refuse hooks before activation. Game camera/WASD addresses are resolved from shared SE-patch instruction patterns, with unique-target and native instruction/reference checks; no game-executable hash whitelist is used. Final updater configuration is default Release (`{}`), without a pin. Exact patch API symmetry, a direct CameraDefinition property, and underlying ECS ClientCamera system reflection remain outside this implementation. The public Ext.ClientCamera Lua namespace is available.

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

The loader must call the exported `Init` function after loading the DLL. Native Mod Loader invokes it before continuing the game's CRT initialization; hook installation completes synchronously there, before SE initializes its native types. A completed static type registry refuses activation before any hooks are installed. Thread-enumeration errors abort installation and retain their actual error codes. Registration snapshot logging occurs after suspended threads resume. This startup requirement adds no Lua capabilities or per-frame work.

SE discovery prefers BG3ScriptExtender.dll. When it is absent, loaded DWrite.dll
modules are checked for the standard SE product name and core DLL description.
The updater and Windows DWrite do not qualify. SE resolution reads only the loaded
DLL. It does not read symtabs, PDBs or debug-directory metadata and contains no
build hashes or address profiles. Native instruction anchors discover candidates;
call relationships distinguish repeated type-getter templates. All derived global
references must agree, and the TLS context displacement is extracted from the
native indexed-load sequence and bounded by PE TLS metadata.
Bridge-owned field offsets, descriptor layouts and strides retain native
instruction witnesses. Required targets must be unique; profiler helpers remain
an optional pair. Missing, ambiguous or incompatible mappings refuse activation
before hooks. Resolution runs once at startup and adds no per-frame work.

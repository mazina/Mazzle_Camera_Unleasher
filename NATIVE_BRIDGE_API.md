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

Native serialization follows SE rules: Ext.Types.Serialize(event) omits pointer fields. Direct JSON serialization requires the normal options for userdata/internal types, such as IterateUserdata=true and StringifyInternalTypes=true; stock default JSON options reject these native objects.

The current normal 34678B65 build is live-tested on published Release C5E9041A; its resolver also passes offline against Devel791C11. The preceding 00F341 build has same-DLL live proof on both published builds. It resolves their matching shipped sibling symtabs and accepts only verified native instruction/layout forms, with bounded helper/global recognition. No SE version/hash whitelist or address profile is used; unsupported or ambiguous forms refuse hooks before activation. The BG3 executable hash guard remains. Final updater configuration is default Release (`{}`), without a pin. Exact patch API symmetry, a direct CameraDefinition property, and underlying ECS ClientCamera system reflection remain outside this implementation. The public Ext.ClientCamera Lua namespace is available.

Camera events and GetCameraDefinition are published under Ext.ClientCamera. Keyboard movement functions are published only under Ext.Input. Ext._Internal.MazzleNativeBridge is not created.

Normal builds contain no statistics counters, counter updates, or diagnostic Lua functions. An intentional maintainer diagnostic build exposes Ext.ClientCamera._Diagnostics; there is no gameplay setting or runtime switch. See README.md for the build flag.

Deploy only MazzleNativeCameraBridgeFull.dll in NativeMods. The Lua registration bootstrap is embedded in the DLL; bridge.lua remains editable source and requires a rebuild when changed. No separate Lua file is needed at runtime.

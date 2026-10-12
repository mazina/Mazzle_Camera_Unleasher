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

Use normal Ext.Events subscriptions and unsubscriptions. SE provides priority, one-shot subscriptions, propagation stopping, custom properties, reflection, validation, and standard object serialization. Calling PreventAction() on CameraInput suppresses native handling and the handled event, returning the event's Result. Finite successful pitch overrides replace the engine result; NaN and infinity retain the original pitch.

event.Camera is ecl::GameCameraBehavior. event.Input is input::InputEvent; its OldValue and NewValue are input::InputValue. All references borrowed from a callback expire when its native callback lifetime ends. Store copied scalar values for later use.

Get the camera definition with:

    local definition = Ext._Internal.MazzleNativeBridge.GetCameraDefinition(event.Camera)

This helper uses SE's normal typed object extraction and returns native CameraGlobalSwitches with the current SE lifetime. Wrong types and expired cameras are rejected through the normal SE checks. Existing camera-mod getter adapters call this helper.

Keyboard movement uses Ext.Input.SetKeyboardMovementOverride(enabled) and Ext.Input.GetKeyboardMovementOverride(). The setter requires a boolean and returns whether the native gate is available. Native movement eligibility and character movement bindings still apply; VM shutdown resets the override.

Native serialization follows SE rules: Ext.Types.Serialize(event) omits pointer fields. Direct JSON serialization requires the normal options for userdata/internal types, such as IterateUserdata=true and StringifyInternalTypes=true; stock default JSON options reject these native objects.

The same native DLL is live-tested on the published Release C5E9041A and Devel791C11 builds. It resolves their matching shipped sibling symtabs and accepts only verified native instruction/layout forms, with bounded helper/global recognition. No SE version/hash whitelist or address profile is used; unsupported or ambiguous forms refuse hooks before activation. The BG3 executable hash guard remains. Final updater configuration is default Release (`{}`), without a pin. Exact patch API symmetry, a direct CameraDefinition property, and ClientCamera system exposure remain outside this implementation.

-- Native payloads and borrowed nested objects are owned by SE.
local bridge=assert(Ext._Internal.MazzleNativeBridge)
local names={'CameraUpdating','CameraUpdated','CameraPitchUpdating','CameraPitchUpdated','CameraZoomUpdated','CameraPitchCalculated','CameraInput','CameraInputHandled','CameraZoomResetting','CameraZoomReset'}
assert(bridge.NativePayloadStatus(),'Native camera event registration was unavailable at startup')
for _,name in ipairs(names) do
    assert(Ext._Internal.EventManager.Events[name]==nil,'Native camera event already published: '..name)
    Ext._Internal.EventManager:RegisterEngineEvent(name)
end
rawset(Ext.Input,'SetKeyboardMovementOverride',bridge.SetKeyboardMovementOverride)
rawset(Ext.Input,'GetKeyboardMovementOverride',bridge.GetKeyboardMovementOverride)
return true

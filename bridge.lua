-- Native payloads and borrowed nested objects are owned by SE.
local camera=assert(Ext.ClientCamera)
local manager=Ext._Internal.EventManager
local names={'CameraUpdating','CameraUpdated','CameraPitchUpdating','CameraPitchUpdated','CameraZoomUpdated','CameraPitchCalculated','CameraInput','CameraInputHandled','CameraZoomResetting','CameraZoomReset'}
for _,name in ipairs(names) do
    assert(manager.Events[name]==nil,'Native camera event already published: '..name)
    manager:RegisterEngineEvent(name)
    camera[name]=manager.Events[name]
end
return true

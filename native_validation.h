#pragma once
#include <windows.h>
namespace NativeValidation {
// Official Release C5 DefaultConstruct<GameCameraBehavior> clears0x258 bytes;
// exact af85fd2 Camera.h compiled with MSVC14.50 reports600 bytes, align8.
constexpr size_t CameraBytes=600;
constexpr size_t InputBytes=36;
inline bool InputPointerReadable(void const* pointer){return pointer==nullptr || !IsBadReadPtr(pointer,InputBytes);}
inline bool CameraPointerReadable(void const* pointer){return pointer==nullptr || !IsBadReadPtr(pointer,CameraBytes);}
// Release ObjectProxy::MakeRef<T> -> StructRegistry -> map fast path.
// Native ValidatePropertyMap alone owns state/bit updates and CheckLevel handling.
template<class Validator>bool AllowProxy(bool globalValidated,int mapState,Validator&& validate){
 if(globalValidated || mapState==1)return true;
 return validate();
}
}

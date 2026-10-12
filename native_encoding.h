#pragma once
#include "lua_value.h"
#include <cstring>
// Official af85 LuaCustomizations.inl CppLightObjectVal + lua_pushlightcppobject.
// Release MakeRef<TLAttachToEventComponent> at0xa4030 corroborates both packed words.
// Only registered uint16 map IDs reach this private bridge helper.
inline TValue EncodeNativeObject(void const* pointer,int mapId,uint64_t lifetime) {
    TValue result{};
    if(mapId<0 || mapId>UINT16_MAX)return result;
    uint64_t raw=reinterpret_cast<uintptr_t>(pointer) | ((lifetime>>32)<<48);
    uint64_t extra=0x80ull | (static_cast<uint64_t>(static_cast<uint16_t>(mapId))<<8) | ((lifetime&0xffffffffull)<<24);
    uint64_t tag=(extra<<8)|9ull;
    std::memcpy(&result,&raw,8);
    std::memcpy(reinterpret_cast<unsigned char*>(&result)+8,&tag,8);
    return result;
}

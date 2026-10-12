#pragma once
#include <cstdint>
// Official custom Lua 5.3 TValue: 8-byte value union + 8-byte tag/extra union.
// Boolean values write only the 32-bit int payload; upper union bytes are unspecified.
struct TValue {
    union { int64_t integer; int32_t boolean; double number; void* pointer; };
    unsigned char tag;
    unsigned char padding[7];
};
static_assert(sizeof(TValue)==16);
inline bool LuaTruthy(TValue const& value) {
    return value.tag!=0 && (value.tag!=1 || value.boolean!=0);
}

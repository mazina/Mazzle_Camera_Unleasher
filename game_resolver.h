#pragma once
#include "runtime_resolver.h"
#include "addresses.h"
// Same camera/WASD signatures used by the SE patch, resolved once at startup.
namespace GameResolver {
using RuntimeResolver::Image;
enum class Id {UpdateCamera,UpdatePitch,UpdateZoom,Definition,Pitch,Input,ResetZoom,KeyboardGate,Count};
struct Resolution {std::array<uint32_t,static_cast<size_t>(Id::Count)> rva{};};
inline constexpr short PatternUpdateCamera[]={0xe8,-1,-1,-1,-1,0x48,0x8d,0x8d,0xf8,0x04,0x00,0x00,0xe8,-1,-1,-1,-1,0xe9,0xaf,0xfd,0xff,0xff};
inline constexpr short PatternUpdatePitch[]={0xf3,0x0f,0x11,0x6d,0xf0,0xe8,-1,-1,-1,-1,0xf2,0x0f,0x10,0x45,0xe8,0x8b,0x45,0xf0,0xf2,0x0f,0x11,0x47,0x70,0x89,0x47,0x78,0x4c,0x8b,0xcb,0x4c,0x8b,0xc7,0x49,0x8b,0xd6,0x49,0x8b,0xcf,0xe8,-1,-1,-1,-1,0x48,0x8b,0x46,0x70};
inline constexpr short PatternUpdateZoom[]={0xe8,-1,-1,-1,-1,0x80,0xbf,0x54,0x02,0x00,0x00,0x00};
inline constexpr short PatternDefinition[]={0x48,0x8b,0x05,-1,-1,-1,-1,0x80,0xb8,0x32,0x13,0x00,0x00,0x00,0x74,0x07};
inline constexpr short PatternPitch[]={0xe8,-1,-1,-1,-1,0x80,0xbf,0x4c,0x01,0x00,0x00,0x00};
inline constexpr short PatternInput[]={0xe8,-1,-1,-1,-1,0x0f,0xb7,0x08,0x66,0x89,0x0b,0x80,0x3b,0x00};
inline constexpr short PatternResetZoom[]={0xe8,-1,-1,-1,-1,0x48,0x8b,0xcf,0xe8,-1,-1,-1,-1,0xe9,0x81,0x02,0x00,0x00};
inline constexpr short PatternKeyboardGate[]={0x80,0x3d,-1,-1,-1,-1,0x00,0x0f,0x84,0x26,0x02,0x00,0x00,0x49,0x8b,0x4f,0x68,0x8b,0x05,-1,-1,-1,-1,0x39,0x81,0x28,0x01,0x00,0x00};
struct Mapping {char const* name;std::span<short const> pattern;uint32_t offset;bool call;std::span<Byte const> entry;};
inline Mapping const Mappings[]={
 {"UpdateCamera",PatternUpdateCamera,0,true,Bytes_UpdateCamera},
 {"UpdatePitch",PatternUpdatePitch,38,true,Bytes_UpdatePitch},
 {"UpdateZoom",PatternUpdateZoom,0,true,Bytes_UpdateZoom},
 {"Definition",PatternDefinition,0,false,{}},
 {"Pitch",PatternPitch,0,true,Bytes_Pitch},
 {"Input",PatternInput,0,true,Bytes_Input},
 {"ResetZoom",PatternResetZoom,0,true,Bytes_ResetZoom},
 {"KeyboardGate",PatternKeyboardGate,0,false,{}},
};
static bool Matches(Image const& image,uint32_t at,std::span<short const> pattern){
 if(!image.Range(at,pattern.size()))return false;
 for(size_t i=0;i<pattern.size();++i)if(pattern[i]>=0 && image.memory[at+i]!=pattern[i])return false;
 return true;
}
static size_t Anchor(std::span<short const> pattern){
 size_t best=0,longest=0;
 for(size_t i=0;i<pattern.size();){if(pattern[i]<0){++i;continue;}size_t begin=i;while(i<pattern.size() && pattern[i]>=0)++i;if(i-begin>longest){longest=i-begin;best=begin;}}
 return best;
}
static bool Resolve(Image const& image,Resolution& result,std::string& why){
 result={};if(!RuntimeResolver::ValidateImage(image,why))return false;
 for(size_t i=0;i<std::size(Mappings);++i){
  auto const& mapping=Mappings[i];size_t anchor=Anchor(mapping.pattern);uint32_t found=0;
  for(auto const& section:image.sections){
   if(!(section.flags&IMAGE_SCN_MEM_EXECUTE) || section.bytes<mapping.pattern.size())continue;
   auto begin=image.memory.data()+section.rva;size_t limit=section.bytes-mapping.pattern.size(),next=0;
   while(next<=limit){
    auto hit=static_cast<Byte const*>(memchr(begin+next+anchor,mapping.pattern[anchor],limit-next+1));if(!hit)break;
    size_t offset=static_cast<size_t>(hit-begin)-anchor;next=offset+1;uint32_t at=section.rva+static_cast<uint32_t>(offset);
    if(!Matches(image,at,mapping.pattern))continue;
    uint32_t target=at+mapping.offset;
    if(mapping.call && !image.SignedTarget(target+5,target+1,target)){why=std::string(mapping.name)+": invalid relative call";return false;}
    if(!image.InSection(target,std::max<size_t>(1,mapping.entry.size()),IMAGE_SCN_MEM_EXECUTE) || (!mapping.entry.empty() && memcmp(image.memory.data()+target,mapping.entry.data(),mapping.entry.size()))){why=std::string(mapping.name)+": unsupported target instructions";return false;}
    if(found && found!=target){why=std::string(mapping.name)+": ambiguous signature";return false;}found=target;
   }
  }
  if(!found){why=std::string(mapping.name)+": signature not found";return false;}
  result.rva[i]=found;
 }
 uint32_t global=0,definition=result.rva[static_cast<size_t>(Id::Definition)],gate=result.rva[static_cast<size_t>(Id::KeyboardGate)];
 if(!image.SignedTarget(definition+7,definition+3,global)||!image.InSection(global,8,IMAGE_SCN_MEM_READ,IMAGE_SCN_MEM_EXECUTE)){why="Definition: global-switches reference invalid";return false;}
 if(!image.SignedTarget(gate+7,gate+2,global)||!image.InSection(global,1,IMAGE_SCN_MEM_READ,IMAGE_SCN_MEM_EXECUTE)){why="KeyboardGate: controller-mode reference invalid";return false;}
 uint32_t rejection=0;
 if(!image.SignedTarget(gate+13,gate+9,rejection)||!image.InSection(rejection,1,IMAGE_SCN_MEM_EXECUTE)){why="KeyboardGate: rejection branch invalid";return false;}
 return true;
}
}

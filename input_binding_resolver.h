#pragma once
#include "game_resolver.h"

// The three input-config locations used by native WASD. Restrict the path
// call to the reload function's PE unwind range: its global signature has
// another, unrelated match in both game renderers.
namespace InputBindingResolver {
using RuntimeResolver::Image;
struct Resolution { uint32_t reload{}, pathCall{}, pathTarget{}, remapCall{}, remapTarget{}; };
inline constexpr short ReloadPattern[]={0x44,0x0f,0xb6,0xfa,0x88,0x55,0xc0,0x48,0x8b,0xf1,0x4c,0x8b,0x35,-1,-1,-1,-1,0x48,0x8b,0x0d,-1,-1,-1,-1,0x48,0x8b,0x01,0xff,0x50,0x20};
inline constexpr short PathPattern[]={0xe8,-1,-1,-1,-1,0x90,0x45,-1,-1,0x45,-1,-1,0x48,-1,-1,0x49};
inline constexpr short RemapPattern[]={0x48,0x83,0xbb,-1,-1,0,0,0,0x74,0x08,0x48,0x8b,0xcb,0xe8,-1,-1,0xea,0xff};
inline constexpr Byte ReloadEntry[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x48,0x89,0x7c,0x24,0x18,0x55};
inline constexpr Byte PathEntry[]={0x48,0x89,0x5c,0x24,0x18,0x48,0x89,0x6c,0x24,0x20,0x48,0x89,0x4c,0x24,0x08,0x56};
inline constexpr Byte RemapEntry[]={0x40,0x55,0x57,0x48,0x83,0xec,0x28,0x48,0x8b,0xb9,0x20,0x08,0,0,0x48,0x8b};
static bool Unique(Image const& image, std::span<short const> pattern, uint32_t& found,
                   uint32_t lower=0, uint32_t upper=UINT32_MAX) {
 found=0;size_t anchor=GameResolver::Anchor(pattern);
 for(auto const& section:image.sections){
  if(!(section.flags&IMAGE_SCN_MEM_EXECUTE))continue;
  uint32_t begin=std::max(lower,section.rva),end=std::min(upper,section.rva+section.bytes);
  if(begin>=end || end-begin<pattern.size())continue;
  size_t limit=end-pattern.size(),next=begin;
  while(next<=limit){
   auto hit=static_cast<Byte const*>(memchr(image.memory.data()+next+anchor,pattern[anchor],limit-next+1));if(!hit)break;
   auto at=static_cast<uint32_t>(hit-image.memory.data()-anchor);next=static_cast<size_t>(at)+1;
   if(!GameResolver::Matches(image,at,pattern))continue;
   if(found)return false;found=at;
  }
 }
 return found!=0;
}
static bool Entry(Image const& image,uint32_t at,std::span<Byte const> bytes){
 return image.InSection(at,bytes.size(),IMAGE_SCN_MEM_EXECUTE) && !memcmp(image.memory.data()+at,bytes.data(),bytes.size());
}
static bool Resolve(Image const& image,Resolution& out,std::string& why){
 out={};if(!RuntimeResolver::ValidateImage(image,why))return false;
 uint32_t hit{};
 if(!Unique(image,ReloadPattern,hit)||hit<45||!Entry(image,hit-45,ReloadEntry)){why="input config reload signature missing, ambiguous or unsupported";return false;}
 out.reload=hit-45;
 IMAGE_DOS_HEADER dos{};image.Read(0,dos);IMAGE_NT_HEADERS64 nt{};image.Read(static_cast<uint32_t>(dos.e_lfanew),nt);
 auto const& exception=nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];uint32_t end{};
 if(!exception.Size || exception.Size%sizeof(RUNTIME_FUNCTION) || !image.InSection(exception.VirtualAddress,exception.Size,IMAGE_SCN_MEM_READ)){why="input config unwind table invalid";return false;}
 for(uint32_t offset=0;offset<exception.Size;offset+=sizeof(RUNTIME_FUNCTION)){
  RUNTIME_FUNCTION entry{};image.Read(exception.VirtualAddress+offset,entry);
  if(entry.BeginAddress==out.reload){if(end){why="duplicate reload unwind range";return false;}end=entry.EndAddress;}
 }
 if(end<=out.reload || !image.InSection(out.reload,end-out.reload,IMAGE_SCN_MEM_EXECUTE)
    || !Unique(image,PathPattern,out.pathCall,out.reload,end)
    || !image.SignedTarget(out.pathCall+5,out.pathCall+1,out.pathTarget) || !Entry(image,out.pathTarget,PathEntry)){
  why="input config path call missing, ambiguous or unsupported";return false;
 }
 if(!Unique(image,RemapPattern,hit)){why="input remap signature missing or ambiguous";return false;}
 out.remapCall=hit+13;
 if(!image.SignedTarget(out.remapCall+5,out.remapCall+1,out.remapTarget)||!Entry(image,out.remapTarget,RemapEntry)){why="input remap target unsupported";return false;}
 return true;
}
}

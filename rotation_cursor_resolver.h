#pragma once
#include "input_binding_resolver.h"

// Original WASD call sites. Resolve independently of SE and refuse the entire
// cursor feature if any site, shared target, import, or virtual slot differs.
namespace RotationCursorResolver {
using RuntimeResolver::Image;
struct Resolution { std::array<uint32_t,6> sites{};uint32_t on{},off{},pollImport{}; };
inline constexpr short Toggle[]={0x41,0x84,-1,0x74,0x07,0xe8,-1,-1,-1,0xff,0xeb,0x05,0xe8,-1,-1,-1,0xff,0x4c,0x8b};
inline constexpr short Off2[]={0x66,0x89,0xb7,0xf0,0x01,0,0,0x48,0x85,0xc9,0x74,-1,0x8d,0x56,0x2e,0xe8};
inline constexpr short Off3[]={0x48,0x85,0xc9,0x74,0x0a,0xba,0x2e,0,0,0,0xe8,-1,-1,-1,0xff,-1,0x8b};
inline constexpr short Poll[]={0x48,0x8d,0x4d,-1,0xff,0x15,-1,-1,-1,0x04,0x85,0xc0,0x0f};
inline constexpr short Virtual[]={0xf3,0x0f,0x11,0x4d,-1,0x4c,-1,-1,-1,0x48,-1,-1,-1,0xff,0x90,0x08,0x01,0,0};
static bool Resolve(Image const& image,Resolution& out,std::string& why){
 out={};uint32_t toggle{},off2{},off3{},poll{},virtualPos{};
 if(!RuntimeResolver::ValidateImage(image,why))return false;
 if(!InputBindingResolver::Unique(image,Toggle,toggle) || !InputBindingResolver::Unique(image,Off2,off2)
    || !InputBindingResolver::Unique(image,Off3,off3) || !InputBindingResolver::Unique(image,Poll,poll)
    || !InputBindingResolver::Unique(image,Virtual,virtualPos)){why="rotation cursor call sites missing, ambiguous or unsupported";return false;}
 out.sites={toggle+5,toggle+12,off2+15,off3+10,poll+4,virtualPos+13};
 for(size_t i=0;i<4;++i){
  uint32_t target{},site=out.sites[i];
  if(!image.InSection(site,5,IMAGE_SCN_MEM_EXECUTE) || image.memory[site]!=0xe8
     || !image.SignedTarget(site+5,site+1,target) || !image.InSection(target,1,IMAGE_SCN_MEM_EXECUTE)){
   why="rotation cursor direct call invalid";return false;
  }
  if(i==0)out.on=target;else if(i==1)out.off=target;else if(target!=out.off){why="rotation release targets disagree";return false;}
 }
 if(out.on==out.off || !image.SignedTarget(out.sites[4]+6,out.sites[4]+2,out.pollImport)
    || !image.InSection(out.pollImport,8,IMAGE_SCN_MEM_READ,IMAGE_SCN_MEM_EXECUTE)){
  why="rotation cursor poll import invalid";return false;
 }
 return true;
}
}

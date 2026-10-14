#pragma once
// Prepared before thread suspension; apply/rollback while game threads remain
// suspended through Detours commit. The selected CALL sites retain their ABI.
struct CallPatch {
 Byte* site{};Byte* relay{};size_t size{};DWORD protection{};bool applied{};
 std::array<Byte,7> original{},replacement{};
 bool Prepare(Byte* address,void* destination,bool keyboard=false){
  site=address;size=keyboard?7:5;if(!keyboard && site[0]!=0xe8)return false;
  memcpy(original.data(),site,size);relay=static_cast<Byte*>(AllocateRelay(reinterpret_cast<uintptr_t>(site)));if(!relay)return false;
  Byte jump[]={0xff,0x25,0,0,0,0};memcpy(relay,jump,sizeof(jump));memcpy(relay+6,&destination,sizeof(destination));DWORD ignored;
  if(!VirtualProtect(relay,14,PAGE_EXECUTE_READ,&ignored))return false;
  FlushInstructionCache(GetCurrentProcess(),relay,14);
  auto difference=reinterpret_cast<intptr_t>(relay)-reinterpret_cast<intptr_t>(site+5);
  if(difference<INT32_MIN || difference>INT32_MAX)return false;
  replacement[0]=0xe8;auto displacement=static_cast<int32_t>(difference);memcpy(replacement.data()+1,&displacement,4);
  if(keyboard){replacement[5]=0x84;replacement[6]=0xc0;}return true;
 }
 bool Apply(){
  if(memcmp(site,original.data(),size) || !VirtualProtect(site,size,PAGE_EXECUTE_READWRITE,&protection))return false;
  memcpy(site,replacement.data(),size);applied=true;DWORD ignored;
  bool restored=VirtualProtect(site,size,protection,&ignored)!=FALSE;
  return FlushInstructionCache(GetCurrentProcess(),site,size)!=FALSE && restored;
 }
 void Rollback(){
  if(applied){DWORD ignored;if(VirtualProtect(site,size,PAGE_EXECUTE_READWRITE,&ignored)){
   memcpy(site,original.data(),size);VirtualProtect(site,size,protection,&ignored);FlushInstructionCache(GetCurrentProcess(),site,size);applied=false;
  }}
 }
 void Discard(){if(relay && !applied){VirtualFree(relay,0,MEM_RELEASE);relay=nullptr;}}
};

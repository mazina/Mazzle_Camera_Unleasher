#pragma once
#include <array>
#include <span>
#include <limits>
#include "resolver_forms.h"
namespace RuntimeResolver {
struct Section {uint32_t rva,bytes,flags;};
struct Symbol {uint32_t rva,bytes;};
struct Resolution {
 std::array<uint32_t,static_cast<size_t>(Id::Count)> rva{};
 std::array<Symbol,static_cast<size_t>(Id::Count)> symbols{};
 std::array<Form const*,static_cast<size_t>(Id::Count)> forms{};
 uint32_t contextOffset=0;uint32_t cameraBytes=0,inputBytes=0,compiledCount=0;bool complete=false;
};
static size_t Slot(Id id){return static_cast<size_t>(id);}
struct Image {
 std::span<Byte const> memory;uintptr_t pointerBase=0;std::vector<Section> sections;
 bool Range(uint32_t r,size_t count)const{return r<memory.size() && count<=memory.size()-r;}
 bool InSection(uint32_t r,size_t count,uint32_t need,uint32_t forbid=0)const{
  if(!Range(r,count))return false;
  for(auto const& s:sections)if(r>=s.rva && count<=s.bytes && r-s.rva<=s.bytes-count && (s.flags&need)==need && !(s.flags&forbid))return true;
  return false;
 }
 template<class T>bool Read(uint32_t r,T& value)const{if(!Range(r,sizeof(T)))return false;memcpy(&value,memory.data()+r,sizeof(T));return true;}
 bool Pointer(uint64_t value,uint32_t& out)const{if(value<pointerBase || value-pointerBase>UINT32_MAX)return false;out=static_cast<uint32_t>(value-pointerBase);return Range(out,1);}
 bool Text(uint32_t r,char const* expected)const{size_t n=strlen(expected)+1;return Range(r,n)&&memcmp(memory.data()+r,expected,n)==0;}
 bool SignedTarget(uint32_t instructionEnd,uint32_t displacement,uint32_t& target)const{
  int32_t d{};if(!Read(displacement,d))return false;int64_t t=static_cast<int64_t>(instructionEnd)+d;
  if(t<0 || t>UINT32_MAX)return false;target=static_cast<uint32_t>(t);return Range(target,1);
 }
};
static bool ValidateImage(Image const& image,std::string& why){
 IMAGE_DOS_HEADER dos{};if(!image.Read(0,dos)||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<0){why="invalid DOS header";return false;}
 IMAGE_NT_HEADERS64 nt{};if(!image.Read(static_cast<uint32_t>(dos.e_lfanew),nt)||nt.Signature!=IMAGE_NT_SIGNATURE||nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC||nt.OptionalHeader.SizeOfImage!=image.memory.size()||nt.FileHeader.SizeOfOptionalHeader!=sizeof(IMAGE_OPTIONAL_HEADER64)||nt.FileHeader.NumberOfSections==0||nt.FileHeader.NumberOfSections>96){why="invalid mapped PE";return false;}
 if(image.sections.size()!=nt.FileHeader.NumberOfSections){why="PE section count mismatch";return false;}
 uint64_t table=static_cast<uint32_t>(dos.e_lfanew)+offsetof(IMAGE_NT_HEADERS64,OptionalHeader)+nt.FileHeader.SizeOfOptionalHeader;
 if(table>UINT32_MAX||!image.Range(static_cast<uint32_t>(table),image.sections.size()*sizeof(IMAGE_SECTION_HEADER))){why="PE section table bounds";return false;}
 for(size_t i=0;i<image.sections.size();++i){IMAGE_SECTION_HEADER raw{};image.Read(static_cast<uint32_t>(table+i*sizeof(raw)),raw);auto const& sec=image.sections[i];
  if(sec.rva!=raw.VirtualAddress||sec.bytes!=std::max(raw.Misc.VirtualSize,raw.SizeOfRawData)||sec.flags!=raw.Characteristics||!image.Range(sec.rva,sec.bytes)){why="PE section bounds mismatch";return false;}
  for(size_t j=0;j<i;++j){auto const& prior=image.sections[j];if(sec.rva<prior.rva+prior.bytes&&prior.rva<sec.rva+sec.bytes){why="overlapping PE sections";return false;}}
 }
 return true;
}
static bool ReadU32(std::span<Byte const> bytes,size_t off,uint32_t& value){if(off>bytes.size()||bytes.size()-off<4)return false;memcpy(&value,bytes.data()+off,4);return true;}
static bool ParseSymbols(Image const& image,std::span<Byte const> table,Resolution& result,std::string& why){
 uint32_t count{};if(!ReadU32(table,0,count)||count==0||count>1000000||count>(table.size()-4)/12){why="invalid symtab header";return false;}
 size_t strings=4+static_cast<size_t>(count)*12;
 for(uint32_t i=0;i<count;++i){uint32_t r{},size{},name{};size_t off=4+static_cast<size_t>(i)*12;
  if(!ReadU32(table,off,r)||!ReadU32(table,off+4,size)||!ReadU32(table,off+8,name)||name>=table.size()-strings){why="symtab record bounds";return false;}
  auto first=reinterpret_cast<char const*>(table.data()+strings+name);auto end=static_cast<char const*>(memchr(first,0,table.size()-strings-name));if(!end){why="unterminated symbol";return false;}
  // Every record must describe executable bytes, not just the names used by the bridge.
  if(size==0||!image.InSection(r,size,IMAGE_SCN_MEM_EXECUTE)){why="symtab executable bounds";return false;}
  for(auto const& wanted:Names)if(strcmp(first,wanted.text)==0){auto slot=Slot(wanted.id);if(result.symbols[slot].bytes){why="duplicate required symbol: "+std::string(first);return false;}result.symbols[slot]={r,size};result.rva[slot]=r;}
 }
 for(auto const& wanted:Names)if(!result.symbols[Slot(wanted.id)].bytes && wanted.id!=Id::FinalizeClass && wanted.id!=Id::ProfilerCtor && wanted.id!=Id::ProfilerDtor){why="missing required symbol: "+std::string(wanted.text);return false;}
 return true;
}
static bool SHA(std::span<Byte const> bytes,char const* expected){
 BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};DWORD size{},got{};bool match=false;
 if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
 if(BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&got,0)>=0){std::vector<Byte> state(size);Byte digest[32]{};
  if(BCryptCreateHash(algorithm,&hash,state.data(),size,nullptr,0,0)>=0){
   if(bytes.size()<=ULONG_MAX && BCryptHashData(hash,const_cast<Byte*>(bytes.data()),static_cast<ULONG>(bytes.size()),0)>=0 && BCryptFinishHash(hash,digest,sizeof(digest),0)>=0){
    char text[65]{};for(unsigned i=0;i<32;++i)snprintf(text+i*2,3,"%02x",digest[i]);match=strcmp(text,expected)==0;
   }BCryptDestroyHash(hash);
  }
 }BCryptCloseAlgorithmProvider(algorithm,0);return match;
}
static bool CheckForms(Image const& image,Resolution& result,std::string& why){
 for(auto const& wanted:Names){auto slot=Slot(wanted.id);auto const& symbol=result.symbols[slot];if(!symbol.bytes)continue;Form const* accepted=nullptr;
  for(auto const& form:Forms)if(form.id==wanted.id && form.bytes==symbol.bytes){std::vector<Byte> code(image.memory.begin()+symbol.rva,image.memory.begin()+symbol.rva+symbol.bytes);bool bounded=true;
   for(size_t i=0;i<form.maskCount;++i){auto m=form.masks[i];if(m.offset>code.size()||m.bytes>code.size()-m.offset){bounded=false;break;}memset(code.data()+m.offset,0,m.bytes);}
   if(bounded && SHA(code,form.sha)){if(accepted){why="ambiguous instruction form";return false;}accepted=&form;}
  }
  if(!accepted){why="unsupported native instruction form: "+std::string(wanted.text);return false;}result.forms[slot]=accepted;
 }
 // Masked relative operands are still checked when they reference a named dependency or a derived global.
 for(auto const& wanted:Names){auto slot=Slot(wanted.id);auto form=result.forms[slot];if(!form)continue;uint32_t start=result.rva[slot];
  for(size_t i=0;i<form->relationCount;++i){auto const& rel=form->relations[i];uint32_t target{};if(!image.SignedTarget(start+rel.end,start+rel.offset,target)){why="relative operand bounds";return false;}
   auto targetSlot=Slot(rel.target);if(rel.target>=Id::TLSIndex){int64_t base=static_cast<int64_t>(target)-rel.addend;if(base<0||base>UINT32_MAX || !image.InSection(static_cast<uint32_t>(base),rel.target==Id::TLSIndex?4:32,IMAGE_SCN_MEM_READ,IMAGE_SCN_MEM_EXECUTE)){why="global operand section/bounds";return false;}
    if(result.rva[targetSlot] && result.rva[targetSlot]!=base){why="inconsistent global references";return false;}result.rva[targetSlot]=static_cast<uint32_t>(base);
   }else if(target!=result.rva[targetSlot]){why="native dependency call mismatch";return false;}
  }
 }
 return true;
}
static bool Match(Image const& image,uint32_t at,std::span<Byte const> pattern,std::span<unsigned const> ignored={}){
 if(!image.Range(at,pattern.size()))return false;
 for(unsigned i=0;i<pattern.size();++i){bool skip=false;for(unsigned x:ignored)if(i>=x&&i<x+4)skip=true;if(!skip&&image.memory[at+i]!=pattern[i])return false;}return true;
}
static bool Finalize(Image const& image,Resolution& result,std::string& why){
 constexpr Byte body[]={0x48,0x8b,0x05,0,0,0,0,0x44,0x8b,0xc2,0x4d,0x03,0xc0,0x4a,0x89,0x0c,0xc0,0xc3};constexpr unsigned bodyMasks[]={3};
 uint32_t helper=result.rva[Slot(Id::FinalizeClass)],staticGlobal=result.rva[Slot(Id::StaticStructs)];unsigned count=0;
 uint32_t knownStatic=staticGlobal;
 if(!helper){
  for(auto const& section:image.sections)if(section.flags&IMAGE_SCN_MEM_EXECUTE)for(uint32_t n=0;n+sizeof(body)<=section.bytes;++n){uint32_t at=section.rva+n,data{};
   if(Match(image,at,body,bodyMasks)&&image.SignedTarget(at+7,at+3,data)&&image.InSection(data,16,IMAGE_SCN_MEM_READ,IMAGE_SCN_MEM_EXECUTE)&&(!knownStatic||data==knownStatic)){++count;helper=at;staticGlobal=data;}
  }
  if(count!=1){why="ambiguous native class-finalization helper";return false;}
 }
 if(!staticGlobal){why="unresolved static registry";return false;}
 // Find the native resize of this exact array. Count is derived, not a fixed source enum value.
 constexpr Byte init[]={0xba,0,0,0,0,0x48,0x8d,0x0d,0,0,0,0,0xe8,0,0,0,0};constexpr unsigned initMasks[]={1,8,13};
 count=0;uint32_t compiledCount{};
 for(auto const& section:image.sections)if(section.flags&IMAGE_SCN_MEM_EXECUTE)for(uint32_t n=0;n+sizeof(init)<=section.bytes;++n){uint32_t at=section.rva+n,call{},data{},size{};
  if(Match(image,at,init,initMasks)&&image.SignedTarget(at+17,at+13,call)&&call==result.rva[Slot(Id::ResizeStructs)]&&image.SignedTarget(at+12,at+8,data)&&data==staticGlobal&&image.Read(at+1,size)&&size>0&&size<=UINT16_MAX){++count;compiledCount=size;}
 }
 if(count!=1){why="ambiguous static registry initializer";return false;}
result.rva[Slot(Id::StaticStructs)]=staticGlobal;result.rva[Slot(Id::FinalizeClass)]=helper;result.compiledCount=compiledCount;return true;
}
static bool ClassDescriptor(Image const& image,char const* name,uint32_t& assign,uint32_t& construct,std::string& why){
 unsigned count=0;for(auto const& s:image.sections)if((s.flags&IMAGE_SCN_MEM_READ)&&!(s.flags&IMAGE_SCN_MEM_EXECUTE))for(uint32_t n=0;n+88<=s.bytes;n+=8){uint32_t at=s.rva+n;uint32_t kind{},id{};uint64_t text{},a{},c{};image.Read(at,kind);image.Read(at+8,text);image.Read(at+32,id);if(kind||id>=UINT16_MAX)continue;uint32_t str{};
  if(!image.Pointer(text,str)||!image.Text(str,name))continue;image.Read(at+64,a);image.Read(at+48,c);if(!image.Pointer(a,assign)||!image.Pointer(c,construct)||!image.InSection(assign,1,IMAGE_SCN_MEM_EXECUTE)||!image.InSection(construct,1,IMAGE_SCN_MEM_EXECUTE)){why="class accessor bounds";return false;}++count;
 }if(count!=1){why="missing/ambiguous native class descriptor";return false;}return true;
}
static bool Layouts(Image const& image,Resolution& result,std::string& why){
 uint32_t assign{},construct{};if(!ClassDescriptor(image,"input::InputEvent",assign,construct,why))return false;
 constexpr Byte copy36[]={0x0f,0x10,0x02,0x0f,0x11,0x01,0x0f,0x10,0x4a,0x10,0x0f,0x11,0x49,0x10,0x8b,0x42,0x20,0x89,0x41,0x20,0xc3};
 if(!image.InSection(assign,sizeof(copy36),IMAGE_SCN_MEM_EXECUTE)||!Match(image,assign,copy36)){why="unsupported InputEvent extent";return false;}result.inputBytes=36;
 if(!ClassDescriptor(image,"ecl::GameCameraBehavior",assign,construct,why)||construct!=result.rva[Slot(Id::CameraConstruct)]){why="camera constructor descriptor mismatch";return false;}
 // Its guarded constructor zeros600 bytes; the unchanged immediate is retained in its native instruction form.
 constexpr Byte extent[]={0x41,0xb8,0x58,0x02,0,0};unsigned extentCount=0;auto f=result.symbols[Slot(Id::CameraConstruct)];for(uint32_t n=0;n+sizeof(extent)<=f.bytes;++n)if(Match(image,f.rva+n,extent))++extentCount;
 if(extentCount!=1){why="camera extent witness";return false;}result.cameraBytes=600;
 // GetCurrentExtensionState retains the actual GS/TLS context access form, with only its global relocation masked.
 auto fctx=result.symbols[Slot(Id::ContextAccessor)];unsigned contexts=0;for(uint32_t n=0;n+5<=fctx.bytes;++n)if(image.memory[fctx.rva+n]==0xb9){uint32_t value{};image.Read(fctx.rva+n+1,value);if(value==0x190||value==0x194){result.contextOffset=value;++contexts;}}
 if(contexts!=1){why="unsupported TLS context accessor";return false;}
 auto dos=reinterpret_cast<IMAGE_DOS_HEADER const*>(image.memory.data());auto nt=reinterpret_cast<IMAGE_NT_HEADERS64 const*>(image.memory.data()+dos->e_lfanew);auto const& tlsDirectory=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];IMAGE_TLS_DIRECTORY64 tls{};uint32_t index{};
 if(tlsDirectory.Size<sizeof(tls)||!image.Read(tlsDirectory.VirtualAddress,tls)||!image.Pointer(tls.AddressOfIndex,index)||index!=result.rva[Slot(Id::TLSIndex)]||tls.EndAddressOfRawData<tls.StartAddressOfRawData||static_cast<uint64_t>(result.contextOffset)+4>tls.EndAddressOfRawData-tls.StartAddressOfRawData+tls.SizeOfZeroFill){why="PE TLS context/index mismatch";return false;}
 for(Id id:{Id::TLSIndex,Id::StructRegistry,Id::StaticStructs})if(!result.rva[Slot(id)]){why="unresolved native global";return false;}
 return true;
}
static bool Resolve(Image const& image,std::span<Byte const> symtab,Resolution& result,std::string& why){result={};if(!ValidateImage(image,why)||!ParseSymbols(image,symtab,result,why)||!CheckForms(image,result,why)||!Finalize(image,result,why)||!Layouts(image,result,why))return false;if(bool(result.rva[Slot(Id::ProfilerCtor)])!=bool(result.rva[Slot(Id::ProfilerDtor)])){why="incomplete native profiler ABI";return false;}result.complete=true;return true;}
}



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
static bool Pattern(Image const& image,uint32_t at,Form const& form){
 if(!image.InSection(at,form.bytes,IMAGE_SCN_MEM_EXECUTE)||!image.Range(at,form.patternBytes))return false;
 for(uint32_t i=0;i<form.patternBytes;++i){bool skip=false;for(size_t j=0;j<form.maskCount;++j){auto m=form.masks[j];if(i>=m.offset && i-m.offset<m.bytes){skip=true;break;}}if(!skip && image.memory[at+i]!=form.pattern[i])return false;}
 auto const& witnesses=FormWitnesses[&form-Forms];
 for(size_t j=0;j<witnesses.count;++j){auto w=witnesses.items[j];if(!image.Range(at+w.offset,w.count)||memcmp(image.memory.data()+at+w.offset,w.bytes,w.count))return false;}
 return true;
}
struct Candidate {uint32_t rva;Form const* form;};
static bool Discover(Image const& image,Resolution& result,std::string& why){
 std::array<std::vector<Candidate>,static_cast<size_t>(Id::TLSIndex)> candidates;
 // PE unwind entries supply function boundaries. Aligned leaf entries are also
 // considered because x64 leaf routines legitimately have no unwind record.
 auto dos=reinterpret_cast<IMAGE_DOS_HEADER const*>(image.memory.data());auto nt=reinterpret_cast<IMAGE_NT_HEADERS64 const*>(image.memory.data()+dos->e_lfanew);
 auto dir=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
 if(!dir.Size || dir.Size%sizeof(RUNTIME_FUNCTION)||!image.Range(dir.VirtualAddress,dir.Size)){why="invalid PE unwind directory";return false;}
 std::vector<RUNTIME_FUNCTION> functions;uint32_t previous=0;
 for(uint32_t off=0;off<dir.Size;off+=sizeof(RUNTIME_FUNCTION)){RUNTIME_FUNCTION f{};image.Read(dir.VirtualAddress+off,f);
  if(f.BeginAddress>=f.EndAddress || f.BeginAddress<previous || !image.InSection(f.BeginAddress,f.EndAddress-f.BeginAddress,IMAGE_SCN_MEM_EXECUTE)||!image.Range(f.UnwindData,4)){why="invalid PE function boundary";return false;}previous=f.EndAddress;functions.push_back(f);
 }
 // Chained unwind records divide optimized functions into owned ranges.
 // Only explicit PE chain ownership may extend a function boundary.
 auto const rawFunctions=functions;std::vector<uint32_t> owners;
 std::vector<RUNTIME_FUNCTION> joined;
 for(auto const& f:functions){RUNTIME_FUNCTION root=f;unsigned depth=0;
  for(;;){uint8_t flags{},codes{};if(!image.Read(root.UnwindData,flags)||!image.Read(root.UnwindData+2,codes)){why="unwind header bounds";return false;}
   if(!(flags>>3&4))break;
   uint32_t tail=root.UnwindData+4+((static_cast<uint32_t>(codes)+1)&~1u)*2;
   RUNTIME_FUNCTION parent{};if(++depth>32||!image.Read(tail,parent)||parent.BeginAddress>=parent.EndAddress){why="invalid unwind chain";return false;}
   auto found=std::lower_bound(functions.begin(),functions.end(),parent.BeginAddress,[](RUNTIME_FUNCTION const& x,uint32_t r){return x.BeginAddress<r;});
   if(found==functions.end()||memcmp(&*found,&parent,sizeof(parent))){why="unwind chain parent mismatch";return false;}root=parent;
  }
  owners.push_back(root.BeginAddress);
 }
 for(size_t i=0;i<functions.size();++i)if(owners[i]==functions[i].BeginAddress)joined.push_back(functions[i]);
 for(size_t i=0;i<functions.size();++i){auto found=std::lower_bound(joined.begin(),joined.end(),owners[i],[](RUNTIME_FUNCTION const& x,uint32_t r){return x.BeginAddress<r;});
  if(found==joined.end()||found->BeginAddress!=owners[i]){why="missing unwind root";return false;}found->EndAddress=std::max(found->EndAddress,functions[i].EndAddress);
 }
 auto owned=[&](uint32_t root,uint32_t at,uint32_t count){uint64_t end=static_cast<uint64_t>(at)+count;
  auto next=std::upper_bound(rawFunctions.begin(),rawFunctions.end(),at,[](uint32_t r,RUNTIME_FUNCTION const& f){return r<f.BeginAddress;});
  if(next==rawFunctions.begin())return false;--next;
  while(static_cast<uint64_t>(at)<end){size_t i=static_cast<size_t>(next-rawFunctions.begin());if(next==rawFunctions.end()||owners[i]!=root||at<next->BeginAddress||at>=next->EndAddress)return false;
   if(end<=next->EndAddress)return true;at=next->EndAddress;++next;
  }return true;
 };
 functions.swap(joined);
 std::vector<uint32_t> starts;for(auto const& f:functions)starts.push_back(f.BeginAddress);
 for(auto const& section:image.sections)if(section.flags&IMAGE_SCN_MEM_EXECUTE){
  uint32_t at=(section.rva+15)&~15u;
  for(;at-section.rva<section.bytes;at+=16){
   auto next=std::upper_bound(rawFunctions.begin(),rawFunctions.end(),at,[](uint32_t r,RUNTIME_FUNCTION const& f){return r<f.BeginAddress;});
   if(next!=rawFunctions.begin()){auto const& prior=*(next-1);if(at>=prior.BeginAddress&&at<prior.EndAddress)continue;}
   starts.push_back(at);
  }
 }
 std::sort(starts.begin(),starts.end());starts.erase(std::unique(starts.begin(),starts.end()),starts.end());
 for(uint32_t at:starts){auto boundary=std::lower_bound(functions.begin(),functions.end(),at,[](RUNTIME_FUNCTION const& f,uint32_t r){return f.BeginAddress<r;});
  for(auto const& form:Forms){if(!Pattern(image,at,form))continue;bool bounded=boundary!=functions.end()&&boundary->BeginAddress==at;
   if(form.unwind&&!bounded)continue;
   if(bounded){bool valid=owned(at,at,form.patternBytes);auto const& witnesses=FormWitnesses[&form-Forms];
    for(size_t i=0;valid&&i<witnesses.count;++i){auto w=witnesses.items[i];valid=owned(at,at+w.offset,w.count);}
    for(size_t i=0;valid&&i<form.relationCount;++i)valid=owned(at,at+form.relations[i].offset,4);
    if(!valid)continue;
   }
   if(!bounded){auto next=std::lower_bound(rawFunctions.begin(),rawFunctions.end(),at,[](RUNTIME_FUNCTION const& f,uint32_t r){return f.BeginAddress<r;});
    if(next!=rawFunctions.end()&&form.bytes>next->BeginAddress-at)continue;
   }
   candidates[Slot(form.id)].push_back({at,&form});}
 }
 auto relationTarget=[&](Candidate const& c,Relation const& rel,uint32_t& target){return image.SignedTarget(c.rva+rel.end,c.rva+rel.offset,target);};
 // Constraint propagation resolves repeated getter templates through their
 // native callers, and checks both outgoing and incoming dependency edges.
 bool changed=true;while(changed){changed=false;
  for(size_t slot=0;slot<candidates.size();++slot){auto& set=candidates[slot];
   auto end=std::remove_if(set.begin(),set.end(),[&](Candidate const& c){
    for(size_t j=0;j<c.form->relationCount;++j){auto rel=c.form->relations[j];uint32_t target{};if(!relationTarget(c,rel,target))return true;
     if(rel.target<Id::TLSIndex){auto const& targets=candidates[Slot(rel.target)];if(std::none_of(targets.begin(),targets.end(),[&](Candidate const& other){return other.rva==target;}))return true;}
     else {int64_t base=static_cast<int64_t>(target)-rel.addend;if(base<=0||base>UINT32_MAX||!image.InSection(static_cast<uint32_t>(base),rel.target==Id::TLSIndex?4:32,IMAGE_SCN_MEM_READ,IMAGE_SCN_MEM_EXECUTE))return true;}
    }
    for(size_t source=0;source<candidates.size();++source){auto const& callers=candidates[source];bool required=false,support=false;
     for(auto const& caller:callers){bool has=false,match=true;for(size_t j=0;j<caller.form->relationCount;++j){auto rel=caller.form->relations[j];if(Slot(rel.target)==slot){has=true;uint32_t target{};if(!relationTarget(caller,rel,target)||target!=c.rva)match=false;}}required=required||has;support=support||(has&&match);}
     if(required&&!support)return true;
    }
    return false;
   });if(end!=set.end()){set.erase(end,set.end());changed=true;}
  }
 }
 for(auto const& wanted:Names){auto slot=Slot(wanted.id);auto const& set=candidates[slot];
  if(set.empty()){if(wanted.id==Id::FinalizeClass||wanted.id==Id::ProfilerCtor||wanted.id==Id::ProfilerDtor)continue;why="missing instruction anchor: "+std::string(wanted.text);return false;}
  uint32_t at=set[0].rva;for(auto const& c:set)if(c.rva!=at){why="ambiguous instruction anchor: "+std::string(wanted.text);return false;}
  auto form=set[0].form;result.rva[slot]=at;result.forms[slot]=form;
  auto boundary=std::lower_bound(functions.begin(),functions.end(),at,[](RUNTIME_FUNCTION const& f,uint32_t r){return f.BeginAddress<r;});
  result.symbols[slot]={at,boundary!=functions.end()&&boundary->BeginAddress==at?boundary->EndAddress-at:form->bytes};
 }
 for(auto const& wanted:Names){auto form=result.forms[Slot(wanted.id)];if(!form)continue;uint32_t start=result.rva[Slot(wanted.id)];
  for(size_t j=0;j<form->relationCount;++j){auto rel=form->relations[j];uint32_t target{};if(!image.SignedTarget(start+rel.end,start+rel.offset,target)){why="dependency bounds";return false;}
   auto slot=Slot(rel.target);if(rel.target<Id::TLSIndex){if(target!=result.rva[slot]){why="dependency mismatch";return false;}}
   else {uint32_t base=static_cast<uint32_t>(static_cast<int64_t>(target)-rel.addend);if(result.rva[slot]&&result.rva[slot]!=base){why="inconsistent native global";return false;}result.rva[slot]=base;}
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
if(!result.symbols[Slot(Id::FinalizeClass)].bytes)result.symbols[Slot(Id::FinalizeClass)]={helper,static_cast<uint32_t>(sizeof(body))};
result.rva[Slot(Id::StaticStructs)]=staticGlobal;result.rva[Slot(Id::FinalizeClass)]=helper;result.compiledCount=compiledCount;return true;
}
static bool ClassDescriptor(Image const& image,char const* name,uint32_t& assign,uint32_t& construct,std::string& why,uint32_t* descriptor=nullptr){
 unsigned count=0;for(auto const& s:image.sections)if((s.flags&IMAGE_SCN_MEM_READ)&&!(s.flags&IMAGE_SCN_MEM_EXECUTE))for(uint32_t n=0;n+88<=s.bytes;n+=8){uint32_t at=s.rva+n;uint32_t kind{},id{};uint64_t text{},a{},c{};image.Read(at,kind);image.Read(at+8,text);image.Read(at+32,id);if(kind||id>=UINT16_MAX)continue;uint32_t str{};
  if(!image.Pointer(text,str)||!image.Text(str,name))continue;image.Read(at+64,a);image.Read(at+48,c);if(!image.Pointer(a,assign)||!image.Pointer(c,construct)||!image.InSection(assign,1,IMAGE_SCN_MEM_EXECUTE)||!image.InSection(construct,1,IMAGE_SCN_MEM_EXECUTE)){why="class accessor bounds";return false;}if(descriptor)*descriptor=at;++count;
 }if(count!=1){why="missing/ambiguous native class descriptor";return false;}return true;
}
static bool Layouts(Image const& image,Resolution& result,std::string& why){
 uint32_t assign{},construct{},eventBase{};if(!ClassDescriptor(image,"lua::EventBase",assign,construct,why,&eventBase))return false;
 // The bridge embeds EventBase. Its inherited Stopped property is accessed by
 // SE's native property getter, so witness its descriptor offset.
 unsigned stoppedCount=0;
 for(uint32_t entry=eventBase+88;image.InSection(entry,88,IMAGE_SCN_MEM_READ,IMAGE_SCN_MEM_EXECUTE);entry+=88){uint32_t kind{},nameRva{},getterRva{};uint64_t name{},offset{},getter{};image.Read(entry,kind);if(kind==0)break;
  if(kind!=3||!image.Read(entry+8,name)||!image.Pointer(name,nameRva)||!image.Text(nameRva,"Stopped"))continue;
  if(!image.Read(entry+48,offset)||offset!=6||!image.Read(entry+16,getter)||!image.Pointer(getter,getterRva)||!image.InSection(getterRva,1,IMAGE_SCN_MEM_EXECUTE)){why="unsupported EventBase Stopped layout";return false;}++stoppedCount;
 }
 if(stoppedCount!=1){why="missing/ambiguous EventBase Stopped descriptor";return false;}
 if(!ClassDescriptor(image,"input::InputEvent",assign,construct,why))return false;
 constexpr Byte copy36[]={0x0f,0x10,0x02,0x0f,0x11,0x01,0x0f,0x10,0x4a,0x10,0x0f,0x11,0x49,0x10,0x8b,0x42,0x20,0x89,0x41,0x20,0xc3};
 if(!image.InSection(assign,sizeof(copy36),IMAGE_SCN_MEM_EXECUTE)||!Match(image,assign,copy36)){why="unsupported InputEvent extent";return false;}result.inputBytes=36;
 if(!ClassDescriptor(image,"ecl::GameCameraBehavior",assign,construct,why)||construct!=result.rva[Slot(Id::CameraConstruct)]){why="camera constructor descriptor mismatch";return false;}
 // Its guarded constructor clears exactly the bridge-owned camera extent.
 constexpr Byte extent[]={0x41,0xb8,0x58,0x02,0,0};unsigned extentCount=0;auto f=result.symbols[Slot(Id::CameraConstruct)];for(uint32_t n=0;n+sizeof(extent)<=f.bytes;++n)if(Match(image,f.rva+n,extent))++extentCount;
 if(extentCount!=1){why="camera extent witness";return false;}result.cameraBytes=600;
 // Extract the TLS context displacement from the validated indexed load sequence;
 // PE TLS metadata supplies its bounds, independent of SE internal member layout.
 auto fctx=result.symbols[Slot(Id::ContextAccessor)];unsigned contexts=0;for(uint32_t n=0;n+5<=fctx.bytes;++n)if(image.memory[fctx.rva+n]==0xb9){uint32_t value{};image.Read(fctx.rva+n+1,value);if(value>0 && n+12<=fctx.bytes && image.memory[fctx.rva+n+5]==0x48 && image.memory[fctx.rva+n+6]==0x8b && image.memory[fctx.rva+n+7]==0x04 && image.memory[fctx.rva+n+8]==0xd0 && image.memory[fctx.rva+n+9]==0x8b && image.memory[fctx.rva+n+10]==0x0c && image.memory[fctx.rva+n+11]==0x01){result.contextOffset=value;++contexts;}}
 if(contexts!=1){why="unsupported TLS context accessor";return false;}
 auto dos=reinterpret_cast<IMAGE_DOS_HEADER const*>(image.memory.data());auto nt=reinterpret_cast<IMAGE_NT_HEADERS64 const*>(image.memory.data()+dos->e_lfanew);auto const& tlsDirectory=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];IMAGE_TLS_DIRECTORY64 tls{};uint32_t index{};
 if(tlsDirectory.Size<sizeof(tls)||!image.Read(tlsDirectory.VirtualAddress,tls)||!image.Pointer(tls.AddressOfIndex,index)||index!=result.rva[Slot(Id::TLSIndex)]||tls.EndAddressOfRawData<tls.StartAddressOfRawData||static_cast<uint64_t>(result.contextOffset)+4>tls.EndAddressOfRawData-tls.StartAddressOfRawData+tls.SizeOfZeroFill){why="PE TLS context/index mismatch";return false;}
 for(Id id:{Id::TLSIndex,Id::StructRegistry,Id::StaticStructs})if(!result.rva[Slot(id)]){why="unresolved native global";return false;}
 return true;
}
static bool Resolve(Image const& image,Resolution& result,std::string& why){result={};if(!ValidateImage(image,why)||!Discover(image,result,why)||!Finalize(image,result,why)||!Layouts(image,result,why))return false;if(bool(result.rva[Slot(Id::ProfilerCtor)])!=bool(result.rva[Slot(Id::ProfilerDtor)])){why="incomplete native profiler ABI";return false;}result.complete=true;return true;}
}



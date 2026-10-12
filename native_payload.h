// Pinned official Release af85fd2 / SHAC5E9041 ABI; mappings in se_abi.h.
// Registration is additive, before any CustomPropertyManager exists.
#include "native_validation.h"
namespace NativePayload {
static uintptr_t& RegistryRva=RVA_StructRegistry;static uintptr_t& StaticStructsRva=RVA_StaticStructs;
struct Array { void* data; uint32_t capacity,size; };
struct Base { uint32_t name=0xffffffff; bool canPrevent=false,prevented=false,stopped=false; uint8_t pad=0; };
struct UpdateEvent:Base {uint64_t entity=0;void* camera=nullptr;float dt=0;};
struct DeltaEvent:Base {void* camera=nullptr;float dt=0;};
struct PitchEvent:Base {void* camera=nullptr;bool ignore=false,select=false;float original=0,pitch=0;};
struct InputEvent:Base {void* camera=nullptr;void* input=nullptr;uint16_t result=0;};
struct ResetEvent:Base {void* camera=nullptr;};
static_assert(offsetof(Base,name)==0 && offsetof(Base,canPrevent)==4 && offsetof(Base,prevented)==5 && offsetof(Base,stopped)==6);
static_assert(offsetof(UpdateEvent,entity)==8 && offsetof(UpdateEvent,camera)==16 && offsetof(UpdateEvent,dt)==24);
static_assert(offsetof(DeltaEvent,camera)==8 && offsetof(DeltaEvent,dt)==16);
static_assert(offsetof(PitchEvent,camera)==8 && offsetof(PitchEvent,ignore)==16 && offsetof(PitchEvent,select)==17 && offsetof(PitchEvent,original)==20 && offsetof(PitchEvent,pitch)==24);
static_assert(offsetof(InputEvent,camera)==8 && offsetof(InputEvent,input)==16 && offsetof(InputEvent,result)==24);
static_assert(offsetof(ResetEvent,camera)==8);
static_assert(sizeof(Base)==8 && sizeof(UpdateEvent)==32 && sizeof(DeltaEvent)==24 && sizeof(PitchEvent)==32 && sizeof(InputEvent)==32 && sizeof(ResetEvent)==16);
using Getter=int(*)(void*,uint64_t,void const*,void const*);
using Setter=int(*)(void*,void*,int,void const*);
using Serializer=int(*)(void*,void const*,void const*);
struct ClassEntry {char const* name;char const* typeName;char const* component;int32_t id,componentType;size_t size;void(*construct)(void*);void(*destroy)(void*);void(*assign)(void*,void*);void(*serialize)(void*,void const*);void(*unserialize)(void*,int,void*);};
struct PropertyEntry {char const* name;Getter get;Setter set;bool(*validate)(void const*,uint64_t);Serializer serialize;size_t offset;uint64_t flag;int32_t notification;char const* newName;bool iterable;};
struct Descriptor {int32_t kind; union {ClassEntry cls;PropertyEntry property;int32_t parent;};};
static_assert(sizeof(ClassEntry)==80 && sizeof(PropertyEntry)==80 && sizeof(Descriptor)==88);

static constexpr unsigned Count=10;
static int firstId=-1,cameraId=-1,inputId=-1,definitionId=-1,parentId=-1;
static void* maps[Count]{};static bool early=false,ready=false;static std::atomic<unsigned> vmCount{0};
static std::atomic<uint64_t> validationCalls[12]{};
using TypeInitialize=void(*)(void*);static TypeInitialize nextTypeInitialize;
using Resize=void(*)(void*,uint32_t);static Resize nextResize;
static char const* names[Count]={"CameraUpdating","CameraUpdated","CameraPitchUpdating","CameraPitchUpdated","CameraZoomUpdated","CameraPitchCalculated","CameraInput","CameraInputHandled","CameraZoomResetting","CameraZoomReset"};
static Array& Registry(){return *reinterpret_cast<Array*>(seBase+RegistryRva);}
static Array& Statics(){return *reinterpret_cast<Array*>(seBase+StaticStructsRva);}
static void* Map(int id){return static_cast<void**>(Registry().data)[id];}
static int FindMap(char const* name){auto& a=Registry();for(uint32_t i=0;i<a.size;++i){auto pm=static_cast<void**>(a.data)[i];if(!pm)continue;auto text=Se<char const*(*)(void const*)>(RVA_FixedStringText)(static_cast<Byte*>(pm)+SeAbi::MapName);if(text && strcmp(text,name)==0)return static_cast<int>(i);}return -1;}

static bool ValidationBit(int id){auto bits=seBase+RegistryRva+SeAbi::RegistryBits;auto words=Field<uint64_t*>(bits,0);return (words[static_cast<unsigned>(id)/64] & (1ull<<(static_cast<unsigned>(id)%64)))!=0;}
static bool CanPush(int id,void const* object){auto pm=Map(id);return NativeValidation::AllowProxy(ValidationBit(id),Field<int32_t>(pm,SeAbi::MapValidated),[&](){if(id==cameraId)++validationCalls[10];else if(id==inputId)++validationCalls[11];else if(id>=firstId && id<firstId+static_cast<int>(Count))++validationCalls[id-firstId];return Se<bool(*)(void*,void const*)>(RVA_ValidateMap)(pm,object);});}
static void PushObject(void* L,int id,void const* ptr,uint64_t lifetime){if(!ptr || !CanPush(id,ptr)){PushNil(L);return;}auto value=EncodeNativeObject(ptr,id,lifetime);auto top=Field<TValue*>(L,SeAbi::LuaTop);*top=value;Field<TValue*>(L,SeAbi::LuaTop)=top+1;}
static int ReadEntity(void* L,uint64_t,void const* value,void const*){auto p=Field<TValue*>(L,SeAbi::LuaTop);p->integer=*static_cast<int64_t const*>(value);reinterpret_cast<uint64_t*>(p)[1]=9ull|(8ull<<8);Field<TValue*>(L,SeAbi::LuaTop)=p+1;return 0;}
static int ReadCamera(void* L,uint64_t lifetime,void const* value,void const*){PushObject(L,cameraId,*static_cast<void*const*>(value),lifetime);return 0;}
static int ReadFloat(void* L,uint64_t,void const* value,void const*){PushNumber(L,*static_cast<float const*>(value));return 0;}
static bool ValidateCamera(void const* value,uint64_t){return NativeValidation::CameraPointerReadable(*static_cast<void*const*>(value));}
static int ReadOnly(void*,void*,int,void const*){return 2;}
static int SerializeEntity(void* L,void const* value,void const*){return ReadEntity(L,0,value,nullptr);}
static int SerializeFloat(void* L,void const* value,void const*){PushNumber(L,*static_cast<float const*>(value));return 0;}
using Validator=bool(*)(void const*,uint64_t);
static Validator FindValidator(void* pm,char const* name){auto& vals=Field<Array>(pm,SeAbi::MapValidators);for(uint32_t i=0;i<vals.size;++i){auto entry=static_cast<Byte*>(vals.data)+static_cast<size_t>(i)*SeAbi::ValidatorStride;auto text=Se<char const*(*)(void const*)>(RVA_FixedStringText)(entry);if(text && strcmp(text,name)==0)return Field<Validator>(entry,SeAbi::ValidatorFunction);}return nullptr;}

static int ReadInput(void* L,uint64_t lifetime,void const* value,void const*){PushObject(L,inputId,*static_cast<void*const*>(value),lifetime);return 0;}
static bool ValidateInput(void const* value,uint64_t){return NativeValidation::InputPointerReadable(*static_cast<void*const*>(value));}
static int ReadBool(void* L,uint64_t,void const* value,void const*){PushBool(L,*static_cast<bool const*>(value));return 0;}
static int ReadResult(void* L,uint64_t,void const* value,void const*){PushInt(L,*static_cast<uint16_t const*>(value));return 0;}
static int SerializeBool(void* L,void const* value,void const*){return ReadBool(L,0,value,nullptr);}
static int SerializeResult(void* L,void const* value,void const*){return ReadResult(L,0,value,nullptr);}
static Setter floatSetter=nullptr,resultSetter=nullptr;
static Setter FindSetter(void* pm,char const* name){auto count=Field<uint32_t>(pm,SeAbi::MapPropertyCount);auto entries=Field<Byte*>(pm,SeAbi::MapProperties);for(uint32_t i=0;i<count;++i){auto e=entries+static_cast<size_t>(i)*SeAbi::PropertyStride;auto text=Se<char const*(*)(void const*)>(RVA_FixedStringText)(e);if(text && strcmp(text,name)==0)return Field<Setter>(e,SeAbi::PropertySetter);}return nullptr;}
template<unsigned I>static void Serialize(void* L,void const* obj){Se<void(*)(void*,void const*,void const*)>(RVA_SerializeRaw)(L,obj,maps[I]);}
template<unsigned I>static void Unserialize(void* L,int index,void* obj){Se<void(*)(void*,int,void*,void const*)>(RVA_UnserializeRaw)(L,index,obj,maps[I]);}
template<unsigned I>using Payload=std::conditional_t<(I<2),UpdateEvent,std::conditional_t<(I<5),DeltaEvent,std::conditional_t<I==5,PitchEvent,std::conditional_t<(I<8),InputEvent,ResetEvent>>>>;
template<unsigned I>static void Construct(void* obj){new(obj) Payload<I>();}
template<unsigned I>static void Destroy(void* obj){Se<void(*)(uint32_t)>(RVA_FixedStringRelease)(static_cast<Payload<I>*>(obj)->name);}
template<unsigned I>static void Assign(void* obj,void* other){auto& out=*static_cast<Payload<I>*>(obj);auto& in=*static_cast<Payload<I>*>(other);struct View{char const* ptr;uint64_t size;};auto text=Se<char const*(*)(void const*)>(RVA_FixedStringText)(&in.name);View v{text,text?strlen(text):0};auto name=text?Se<uint32_t(*)(View const*)>(RVA_FixedStringCreate)(&v):0xffffffffu;Se<void(*)(uint32_t)>(RVA_FixedStringRelease)(out.name);out=in;out.name=name;}
static void ResizeStatics(void* self,uint32_t size){if(early && self==seBase+StaticStructsRva && Statics().size==0 && size==static_cast<uint32_t>(firstId)){nextResize(self,size+Count);return;}nextResize(self,size);}
static void* TypeRef(int id){return static_cast<Byte*>(Statics().data)+static_cast<size_t>(id)*SeAbi::StaticTypeStride;}
static void Member(void* ty,char const* name,void* ref){Se<void(*)(void*,char const*,void**)>(RVA_AddMember)(ty,name,&ref);}
template<unsigned I>static void RegisterMap(){
 std::string full=std::string("ecl::lua::")+names[I]+"Event";
 std::vector<Descriptor> ds;Descriptor cls{};cls.kind=0;cls.cls={full.c_str(),full.c_str(),nullptr,firstId+static_cast<int>(I),-1,sizeof(Payload<I>),Construct<I>,Destroy<I>,Assign<I>,Serialize<I>,Unserialize<I>};ds.push_back(cls);
 Descriptor parent{};parent.kind=1;parent.parent=parentId;ds.push_back(parent);
 auto prop=[&](char const* name,Getter get,Setter set,Validator validate,Serializer serialize,size_t offset){Descriptor d{};d.kind=3;d.property={name,get,set,validate,serialize,offset,0,0,nullptr,true};ds.push_back(d);};
 if constexpr(I<2)prop("Entity",ReadEntity,ReadOnly,FindValidator(Map(cameraId),"Trigger"),SerializeEntity,8);
 prop("Camera",ReadCamera,ReadOnly,ValidateCamera,nullptr,I<2?16:8);
 if constexpr(I<5)prop("DeltaTime",ReadFloat,ReadOnly,FindValidator(Map(cameraId),"ZoomSpeed"),SerializeFloat,I<2?24:16);
 if constexpr(I==5){prop("IgnoreAvoidance",ReadBool,ReadOnly,FindValidator(Map(cameraId),"PlayerInControl"),SerializeBool,16);prop("SelectMode",ReadBool,ReadOnly,FindValidator(Map(cameraId),"PlayerInControl"),SerializeBool,17);prop("OriginalPitch",ReadFloat,ReadOnly,FindValidator(Map(cameraId),"ZoomSpeed"),SerializeFloat,20);prop("Pitch",ReadFloat,floatSetter,FindValidator(Map(cameraId),"ZoomSpeed"),SerializeFloat,24);}
 if constexpr(I==6 || I==7){prop("Input",ReadInput,ReadOnly,ValidateInput,nullptr,16);prop("Result",ReadResult,I==6?resultSetter:ReadOnly,FindValidator(Map(inputId),"DeviceId"),SerializeResult,24);}
 Descriptor end{};end.kind=5;ds.push_back(end);Se<void(*)(Descriptor const*)>(RVA_ProcessDefinitions)(ds.data());maps[I]=Map(firstId+I);Se<void(*)(void const*,void*)>(RVA_InheritProperties)(Map(parentId),maps[I]);
}
template<unsigned I>static void RegisterType(){
 std::string full=std::string("ecl::lua::")+names[I]+"Event";std::string native=std::string("struct bg3se::")+full;
 auto ty=Se<void*(*)(char const*,char const*,void*,char const*,char const*)>(RVA_RegisterClass)(full.c_str(),native.c_str(),maps[I],nullptr,nullptr);Field<void*>(ty,SeAbi::TypeParent)=TypeRef(parentId);
 if constexpr(I<2)Member(ty,"Entity",Se<void*(*)()>(RVA_StaticEntityType)());Member(ty,"Camera",TypeRef(cameraId));
 if constexpr(I<5)Member(ty,"DeltaTime",Se<void*(*)()>(RVA_StaticFloatType)());
 if constexpr(I==5){Member(ty,"IgnoreAvoidance",Se<void*(*)()>(RVA_StaticBoolType)());Member(ty,"SelectMode",Se<void*(*)()>(RVA_StaticBoolType)());Member(ty,"OriginalPitch",Se<void*(*)()>(RVA_StaticFloatType)());Member(ty,"Pitch",Se<void*(*)()>(RVA_StaticFloatType)());}
 if constexpr(I==6 || I==7){Member(ty,"Input",TypeRef(inputId));Member(ty,"Result",Se<void*(*)()>(RVA_StaticUInt16Type)());}
 Se<void(*)(void*,int)>(RVA_FinalizeClass)(ty,firstId+I);
}
template<size_t... I>static void AllMaps(std::index_sequence<I...>){(RegisterMap<I>(),...);}
template<size_t... I>static void AllTypes(std::index_sequence<I...>){(RegisterType<I>(),...);}
static void InitializeTypes(void* repository){
 auto& reg=Registry();auto& st=Statics();
 if(vmCount.load() || reg.size==0 || reg.size+Count>UINT16_MAX || st.size!=0 || firstId>=0){Log("native events: late/duplicate startup refused");nextTypeInitialize(repository);return;}
 parentId=FindMap("lua::EventBase");cameraId=FindMap("ecl::GameCameraBehavior");inputId=FindMap("input::InputEvent");definitionId=FindMap("CameraGlobalSwitches");
 if(parentId<0 || cameraId<0 || inputId<0 || definitionId<0){Log("native events: required official maps missing");nextTypeInitialize(repository);return;}
 floatSetter=FindSetter(Map(cameraId),"ZoomSpeed");resultSetter=FindSetter(Map(inputId),"DeviceId");
 if(!floatSetter || !resultSetter || !FindValidator(Map(cameraId),"Trigger") || !FindValidator(Map(cameraId),"ZoomSpeed") || !FindValidator(Map(cameraId),"PlayerInControl") || !FindValidator(Map(inputId),"DeviceId")){Log("native events: official scalar accessors missing");nextTypeInitialize(repository);return;}
 firstId=static_cast<int>(reg.size);auto newSize=reg.size+Count;auto buffer=Se<void*(*)(size_t)>(RVA_Alloc)(static_cast<size_t>(newSize)*sizeof(void*));if(!buffer){firstId=-1;nextTypeInitialize(repository);return;}
 memcpy(buffer,reg.data,static_cast<size_t>(reg.size)*sizeof(void*));memset(static_cast<void**>(buffer)+reg.size,0,Count*sizeof(void*));
 auto bits=seBase+RegistryRva+SeAbi::RegistryBits;auto required=(newSize+63u)&~63u;auto oldBitSize=Field<uint32_t>(bits,SeAbi::BitsetSize);void* newBits=nullptr;
 if(oldBitSize<required){newBits=Se<void*(*)(size_t)>(RVA_Alloc)(required/8);if(!newBits){Se<void(*)(void*)>(RVA_Free)(buffer);firstId=-1;nextTypeInitialize(repository);return;}memset(newBits,0,required/8);memcpy(newBits,Field<void*>(bits,0),oldBitSize/8);}
 auto old=reg.data;reg.data=buffer;reg.capacity=newSize;reg.size=newSize;Se<void(*)(void*)>(RVA_Free)(old);
 if(newBits){auto oldBits=Field<void*>(bits,0);Field<void*>(bits,0)=newBits;Field<uint32_t>(bits,SeAbi::BitsetSize)=required;Se<void(*)(void*)>(RVA_Free)(oldBits);}
 early=true;AllMaps(std::make_index_sequence<Count>{});nextTypeInitialize(repository);if(st.size!=reg.size){Log("native events: static registry mismatch; dispatch refused");return;}
 AllTypes(std::make_index_sequence<Count>{});ready=true;Log("native events: all ten registered before type Finalize and Lua VMs");
}
struct Result{int status=1;float pitch=0;uint16_t input=0;};
template<unsigned I>static Result DispatchOne(void* state,void* L,void* camera,float dt,uint64_t entity,void* input,float original,bool ignore,bool select,uint64_t lifetime){
 Payload<I> evt{};evt.camera=camera;
 if constexpr(I<2){evt.entity=entity;evt.dt=dt;}else if constexpr(I<5)evt.dt=dt;
 if constexpr(I==5){evt.original=original;evt.pitch=original;evt.ignore=ignore;evt.select=select;}
 if constexpr(I==6 || I==7){evt.input=input;evt.result=I==6?0:static_cast<uint16_t>(original);}
 Se<void(*)(void*,char const*)>(RVA_PushInternalFunction)(L,"_ThrowEvent");PushObject(L,firstId+I,&evt,lifetime);
 Result ret;ret.status=Se<int(*)(void*,Base*,char const*,bool,uint32_t)>(RVA_DispatchEvent)(state,&evt,names[I],I==6,0);
 if constexpr(I==5)ret.pitch=evt.pitch;if constexpr(I==6 || I==7)ret.input=evt.result;
 Se<void(*)(uint32_t)>(RVA_FixedStringRelease)(evt.name);return ret;
}
static Result Dispatch(unsigned i,void* state,void* L,void* camera,float dt,uint64_t entity,void* input,float original,bool ignore,bool select,uint64_t lifetime){
 if(!ready)return {};switch(i){
#define EVENT_CASE(I) case I:return DispatchOne<I>(state,L,camera,dt,entity,input,original,ignore,select,lifetime)
 EVENT_CASE(0);EVENT_CASE(1);EVENT_CASE(2);EVENT_CASE(3);EVENT_CASE(4);EVENT_CASE(5);EVENT_CASE(6);EVENT_CASE(7);EVENT_CASE(8);EVENT_CASE(9);
#undef EVENT_CASE
 default:return {};}
}
static int Definition(void* L){if(!ready){PushNil(L);return 1;}auto camera=Se<void*(*)(void*,int,void const*)>(RVA_GetRawObject)(L,1,Map(cameraId));auto def=reinterpret_cast<void*(*)(void const*)>(gameBase+RVA_Definition)(camera);PushObject(L,definitionId,def,Field<uint64_t>(static_cast<Byte*>(L)-SeAbi::ExtraLifetimeBack,0));return 1;}
static int ValidationStatus(void* L){if(!Se<int(*)(void*,int)>(RVA_CheckStack)(L,Count*3))return Se<int(*)(void*,char const*,...)>(RVA_LuaError)(L,"cannot reserve native validation status stack");for(unsigned i=0;i<Count;++i){PushInt(L,ready?Field<int32_t>(maps[i],SeAbi::MapValidated):-1);PushBool(L,ready&&ValidationBit(firstId+i));PushInt(L,validationCalls[i].load());}return Count*3;}
static int Status(void* L){PushBool(L,ready);PushInt(L,firstId);PushInt(L,cameraId);PushInt(L,parentId);PushInt(L,vmCount.load());PushInt(L,inputId);PushInt(L,definitionId);return 7;}
}


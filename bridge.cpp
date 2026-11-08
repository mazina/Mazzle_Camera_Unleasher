#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include "bootstrap_resource.h"
#include <tlhelp32.h>
#include <intrin.h>
#include <detours.h>
#include <atomic>
#include <type_traits>
#include <utility>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <share.h>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
using Byte=unsigned char;
#include "addresses.h"
#include "runtime_module.h"
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"uuid.lib")
static Byte* seBase; static Byte* gameBase;
static std::atomic<void*> clientExt{};
static std::atomic<void*> installedState{}; static std::atomic<bool> enabled{false};
static std::atomic<bool> dispatchFailureReported{false};
static FILE* logFile;static SRWLOCK logLock=SRWLOCK_INIT;
// Diagnostics are deliberately absent from normal builds, including counter updates.
#ifndef MAZZLE_BRIDGE_DIAGNOSTICS
#define MAZZLE_BRIDGE_DIAGNOSTICS 0
#endif
#if MAZZLE_BRIDGE_DIAGNOSTICS
static std::atomic<unsigned> generation{};
static std::atomic<unsigned long long> calls[10]{},errors{},prevented{},overrides{};
static constexpr unsigned long long wrongThread=0;
#define BRIDGE_STAT(expression) do { expression; } while (false)
#else
#define BRIDGE_STAT(expression) ((void)0)
#endif
static std::string companion;
static HINSTANCE bridgeInstance{};
static std::atomic_flag startupRequested=ATOMIC_FLAG_INIT;
extern "C" { std::atomic<uint8_t> KeyboardMovementOverride{0}; uint8_t const* KeyboardMovementControllerMode=nullptr; void KeyboardMovementModeBridge(); }
static_assert(sizeof(KeyboardMovementOverride)==1);
static std::atomic<bool> keyboardAvailable{false};
static void Log(char const* message){AcquireSRWLockExclusive(&logLock);if(logFile){fprintf(logFile,"%llu %s\n",GetTickCount64(),message);fflush(logFile);}ReleaseSRWLockExclusive(&logLock);}
static bool OpenLog(){
 PWSTR appData=nullptr;
 if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,KF_FLAG_DEFAULT,nullptr,&appData)))return false;
 std::wstring directory=appData;CoTaskMemFree(appData);
 directory+=LR"(\Larian Studios\Baldur's Gate 3\Script Extender)";
 int result=SHCreateDirectoryExW(nullptr,directory.c_str(),nullptr);
 if(result!=ERROR_SUCCESS && result!=ERROR_ALREADY_EXISTS && result!=ERROR_FILE_EXISTS)return false;
 auto path=directory+L"\\MazzleNativeCameraBridgeFull.log";
 logFile=_wfsopen(path.c_str(),L"w",_SH_DENYNO);
 return logFile!=nullptr;
}
// Use the existing Windows console, including when SE ABI resolution has failed.
// Never allocate another console or route startup output through unresolved SE functions.
static void ReportStartup(char const* message){
 Log(message);
 std::string line="[Camera Unleasher] ";line+=message;line+="\r\n";
 auto output=GetStdHandle(STD_OUTPUT_HANDLE);DWORD written{};
 if(output && output!=INVALID_HANDLE_VALUE)WriteConsoleA(output,line.c_str(),static_cast<DWORD>(line.size()),&written,nullptr);
}
static void ReportFailure(std::string const& reason){ReportStartup(("FAILED: "+reason).c_str());}
template<class T>T& Field(void* p,size_t offset){return *reinterpret_cast<T*>(static_cast<Byte*>(p)+offset);}
template<class F>F Se(uintptr_t rva){return reinterpret_cast<F>(seBase+rva);}
struct SeString{union{char const* ptr;char inlineData[16];};uint32_t size,capacity;};
struct OptionalInt{int value;bool hasValue;Byte padding[3];};
static SeString Borrow(char const* text){SeString s{};s.size=static_cast<uint32_t>(strlen(text));if(s.size<=15){memcpy(s.inlineData,text,s.size+1);s.capacity=15;}else{s.ptr=text;s.capacity=s.size;}return s;}
#include "lua_value.h"
#include "native_encoding.h"
#include "runtime_resolver.h"

#include "game_resolver.h"
static_assert(sizeof(TValue)==16);
static TValue* At(void* L,int index){return Se<TValue*(*)(void*,int)>(RVA_Index)(L,index);}
static int Top(void* L){return static_cast<int>((Field<Byte*>(L,SeAbi::LuaTop)-Field<Byte*>(Field<void*>(L,SeAbi::LuaCallInfo),0))/16)-1;}
static void SetTop(void* L,int value){Se<void(*)(void*,int)>(RVA_SetTop)(L,value);}
static void PushInt(void* L,int64_t value){auto p=Field<TValue*>(L,SeAbi::LuaTop);p->integer=value;p->tag=19;Field<TValue*>(L,SeAbi::LuaTop)=p+1;}
static void PushNumber(void* L,double value){auto p=Field<TValue*>(L,SeAbi::LuaTop);p->number=value;p->tag=3;Field<TValue*>(L,SeAbi::LuaTop)=p+1;}
static void PushBool(void* L,bool value){auto p=Field<TValue*>(L,SeAbi::LuaTop);p->integer=value;p->tag=1;Field<TValue*>(L,SeAbi::LuaTop)=p+1;}
static void PushNil(void* L){auto p=Field<TValue*>(L,SeAbi::LuaTop);p->integer=0;p->tag=0;Field<TValue*>(L,SeAbi::LuaTop)=p+1;}
static bool Boolean(void* L,int index){return LuaTruthy(*At(L,index));}
// Official Release LoadScript inlines this exact LifetimeStackPin constructor.
// Pool::Allocate returns through its hidden output argument; native destructor owns pop/kill/restore.
static void InitializeLifetimePin(void* pin,void* L,void* stack){
 if(RVA_LifetimeCtor){Se<void*(*)(void*,void*,void*)>(RVA_LifetimeCtor)(pin,L,stack);return;}
 Field<void*>(pin,0)=stack;
 auto current=reinterpret_cast<uint64_t*>(static_cast<Byte*>(L)-SeAbi::ExtraLifetimeBack);
 Field<uint64_t*>(pin,SeAbi::PinCurrentReference)=current;
 Field<uint64_t>(pin,SeAbi::PinPrevious)=*current;
 uint64_t handle{};
 Se<uint64_t*(*)(void*,uint64_t*)>(RVA_LifetimeAllocate)(Field<void*>(stack,0),&handle);
 Se<void(*)(void*,uint64_t const*)>(RVA_LifetimePush)(static_cast<Byte*>(stack)+SeAbi::LifetimeStackVector,&handle);
 *current=handle;
}
static OptionalInt Load(void* state,char const* text,char const* label){auto code=Borrow(text),name=Borrow(label);OptionalInt ret{};Se<OptionalInt*(*)(void*,OptionalInt*,SeString const*,SeString const*,int)>(RVA_LoadScript)(state,&ret,&code,&name,0);return ret;}
static int KeyboardSet(void* L){if(At(L,1)->tag!=1)return Se<int(*)(void*,int,char const*)>(RVA_ArgError)(L,1,"boolean expected");if(keyboardAvailable)KeyboardMovementOverride.store(Boolean(L,1),std::memory_order_relaxed);PushBool(L,keyboardAvailable);return 1;}
static int KeyboardGet(void* L){PushBool(L,KeyboardMovementOverride.load(std::memory_order_relaxed)!=0);return 1;}
#if MAZZLE_BRIDGE_DIAGNOSTICS
static int Statistics(void* L){for(auto& count:calls)PushInt(L,static_cast<int64_t>(count.load()));PushInt(L,static_cast<int64_t>(errors.load()));PushInt(L,static_cast<int64_t>(wrongThread));PushInt(L,generation.load());PushInt(L,static_cast<int64_t>(prevented.load()));PushInt(L,static_cast<int64_t>(overrides.load()));return 15;}
#endif
static void Bind(void* L,char const* name,int(*fn)(void*)){Se<void(*)(void*,char const*)>(RVA_PushString)(L,name);Se<void(*)(void*,int(*)(void*),int)>(RVA_PushClosure)(L,fn,0);Se<void(*)(void*,int)>(RVA_SetTable)(L,-3);}
#include "input_bindings.h"
#include "native_payload.h"
#include "cursor_visibility.h"
#include "rotation_cursor.h"
using InitializeFn=void(*)(void*);static InitializeFn nextInitialize;
static void InitializeState(void* state){
 ++NativePayload::vmCount;nextInitialize(state);installedState=state;enabled=false;
 dispatchFailureReported.store(false,std::memory_order_relaxed);
 if(!NativePayload::ready){NativePayload::LogRegistrationState("client bindings refused");ReportFailure("native camera registration unavailable; bindings refused");return;}
 void* L=Field<void*>(state,SeAbi::StateLua);int top=Top(L);
 if(!Se<int(*)(void*,int)>(RVA_CheckStack)(L,64)){ReportFailure("cannot reserve Lua stack");return;}
 auto failed=[&](){SetTop(L,top);ReportFailure("binding table creation failed");};
 auto ret=Load(state,"local camera={}; rawset(Ext,'ClientCamera',camera); return camera","NativeCameraBindings");
 if(!ret.hasValue || ret.value!=1){failed();return;}
 Bind(L,"GetCameraDefinition",NativePayload::Definition);SetTop(L,top);
 ret=Load(state,"return Ext.Input","NativeInputBindings");
 if(!ret.hasValue || ret.value!=1){failed();return;}
 CursorVisibility::Initialize();
 Bind(L,"SetKeyboardMovementOverride",KeyboardSet);Bind(L,"GetKeyboardMovementOverride",KeyboardGet);Bind(L,"SetCursorHidden",CursorVisibility::Set);Bind(L,"SetRotationCursorEnabled",RotationCursor::Set);SetTop(L,top);
 ret=Load(state,"return Ext.Input","NativeMovementBindingSetup");
 if(!ret.hasValue || ret.value!=1){failed();return;}
 Bind(L,"SyncKeyboardMovementBindings",InputBindings::Synchronize);Bind(L,"GetKeyboardMovementBindings",InputBindings::Snapshot);Bind(L,"GetKeyboardMovementBindingVersion",InputBindings::Revision);SetTop(L,top);
#if MAZZLE_BRIDGE_DIAGNOSTICS
 ret=Load(state,"local diagnostics={}; Ext.ClientCamera._Diagnostics=diagnostics; return diagnostics","NativeCameraDiagnostics");
 if(!ret.hasValue || ret.value!=1){failed();return;}
 Bind(L,"NativeStatistics",Statistics);Bind(L,"NativePayloadStatus",NativePayload::Status);Bind(L,"NativePayloadValidationStatus",NativePayload::ValidationStatus);SetTop(L,top);
#endif
 ret=Load(state,companion.c_str(),"NativeBridgeCompanion");
 if(ret.hasValue && ret.value==1){enabled=true;Log("all native camera events registered before mod bootstrap");ReportStartup("LOADED: native camera and input API initialized.");}
 else ReportFailure("embedded Lua bootstrap initialization failed; native fallback retained");
 SetTop(L,top);
}
using UpdateFn=void(*)(void*,void const*);static UpdateFn nextUpdate;
static void Update(void* ext,void const* time){clientExt.store(ext);nextUpdate(ext,time);CursorVisibility::Update();}
using ShutdownFn=void(*)(void*);static ShutdownFn nextShutdown;
static void Shutdown(void* ext){enabled=false;InputBindings::Shutdown();installedState=nullptr;clientExt.store(nullptr);KeyboardMovementOverride.store(0);BRIDGE_STAT(++generation);Log("client VM shutdown: dispatch ref invalidated; keyboard override false");nextShutdown(ext);KeyboardMovementOverride.store(0);CursorVisibility::Shutdown();RotationCursor::Shutdown();}
struct DispatchResult{bool successful=false;float value=0;bool hasNumber=false;bool prevented=false;uint16_t inputResult=0;};
static DispatchResult Dispatch(int event,void* camera,float dt,uint64_t entity=0,void* input=nullptr,double original=0,bool ignore=false,bool select=false){
 DispatchResult result;void* ext=clientExt.load();if(!ext)return result;
 auto tls=reinterpret_cast<Byte**>(__readgsqword(0x58));auto tlsIndex=*reinterpret_cast<DWORD*>(seBase+RVA_TLSIndex);int& context=*reinterpret_cast<int*>(tls[tlsIndex]+TLS_ContextOffset);int priorContext=context;context=2;
 Se<void(*)(void*)>(RVA_IncRefs)(ext);void* state=Field<void*>(ext,SeAbi::ExtensionLua);
 if(enabled.load() && !Field<bool>(ext,SeAbi::ExtensionResetPending) && state && state==installedState.load()){
  void* L=Field<void*>(state,SeAbi::StateLua);int top=Top(L);
  if(Se<int(*)(void*,int)>(RVA_CheckStack)(L,64)){
   alignas(8) Byte lifetime[SeAbi::PinBytes]{};alignas(8) Byte profiler[8]{};
   InitializeLifetimePin(lifetime,L,static_cast<Byte*>(state)+SeAbi::StateLifetimeStack);
   if(RVA_ProfilerCtor)Se<void(*)(void*,void*)>(RVA_ProfilerCtor)(profiler,state);
   auto native=NativePayload::Dispatch(static_cast<unsigned>(event),state,L,camera,dt,entity,input,static_cast<float>(original),ignore,select,*Field<uint64_t*>(lifetime,SeAbi::PinCurrentReference));BRIDGE_STAT(++calls[event]);
   result.successful=native.status==0;result.prevented=native.status==2;result.value=native.pitch;result.hasNumber=event==5;result.inputResult=native.input;
   if(native.status==1){BRIDGE_STAT(++errors);if(!dispatchFailureReported.exchange(true,std::memory_order_relaxed))Log("native event dispatch failed; engine fallback retained; further failures suppressed until the next client VM");}
   SetTop(L,top);if(RVA_ProfilerDtor)Se<void(*)(void*)>(RVA_ProfilerDtor)(profiler);Se<void(*)(void*)>(RVA_LifetimeDtor)(lifetime);
  }
 }
 Se<void(*)(void*,bool)>(RVA_DecRefs)(ext,true);context=priorContext;return result;
}
using UpdateCameraFn=void(*)(void*,void*,void const*,void*);static UpdateCameraFn nextCamera;
static void Camera(void* self,void* context,void const* time,void* view){auto camera=Field<void*>(view,0x30);auto dt=Field<float>(const_cast<void*>(time),0xc);auto entity=Field<uint64_t>(view,0x70);Dispatch(0,camera,dt,entity);nextCamera(self,context,time,view);Dispatch(1,Field<void*>(view,0x30),Field<float>(const_cast<void*>(time),0xc),Field<uint64_t>(view,0x70));}
using UpdatePitchFn=void(*)(void*,void*,void*,void const*);static UpdatePitchFn nextUpdatePitch;
static void UpdatePitch(void* self,void* entity,void* camera,void const* time){auto dt=Field<float>(const_cast<void*>(time),0xc);Dispatch(2,camera,dt);nextUpdatePitch(self,entity,camera,time);Dispatch(3,camera,Field<float>(const_cast<void*>(time),0xc));}
using UpdateZoomFn=void(*)(void*,void*,void*,void const*);static UpdateZoomFn nextZoom;
static void Zoom(void* self,void* entity,void* view,void const* time){nextZoom(self,entity,view,time);Dispatch(4,Field<void*>(view,0x10),Field<float>(const_cast<void*>(time),0xc));}
using PitchFn=float(*)(void*,bool,bool);static PitchFn nextPitch;
static float Pitch(void* camera,bool ignore,bool select){float pitch=nextPitch(camera,ignore,select);auto result=Dispatch(5,camera,0,0,nullptr,pitch,ignore,select);if(result.successful && result.hasNumber && std::isfinite(result.value) && std::isfinite(static_cast<float>(result.value))){BRIDGE_STAT(++overrides);return static_cast<float>(result.value);}return pitch;}
using InputFn=uint16_t*(*)(void*,uint16_t*,void*,void*);static InputFn nextInput;
static uint16_t* Input(void* self,uint16_t* result,void* view,void* input){auto camera=Field<void*>(view,0x40);auto event=Dispatch(6,camera,0,0,input);if(event.prevented){*result=event.inputResult;BRIDGE_STAT(++prevented);return result;}auto ret=nextInput(self,result,view,input);Dispatch(7,Field<void*>(view,0x40),0,0,input,*ret);return ret;}
using ResetZoomFn=void(*)(void*);static ResetZoomFn nextReset;
static void ResetZoom(void* view){auto camera=Field<void*>(view,0x20);Dispatch(8,camera,0);nextReset(view);Dispatch(9,Field<void*>(view,0x20),0);}
static void* AllocateRelay(uintptr_t site){SYSTEM_INFO info;GetSystemInfo(&info);auto granularity=static_cast<uintptr_t>(info.dwAllocationGranularity);auto lower=site>0x7fff0000ull?site-0x7fff0000ull:granularity;auto upper=site+0x7fff0000ull;auto address=(lower+granularity-1)&~(granularity-1);while(address<upper){MEMORY_BASIC_INFORMATION region;if(!VirtualQuery(reinterpret_cast<void*>(address),&region,sizeof(region)))break;auto end=reinterpret_cast<uintptr_t>(region.BaseAddress)+region.RegionSize;if(region.State==MEM_FREE && end-address>=info.dwPageSize){if(auto relay=VirtualAlloc(reinterpret_cast<void*>(address),info.dwPageSize,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE))return relay;}if(end<=address)break;address=(end+granularity-1)&~(granularity-1);}return nullptr;}
#include "call_patch.h"
static LONG EnlistThreads(std::vector<HANDLE>& opened){
 HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);if(snap==INVALID_HANDLE_VALUE)return static_cast<LONG>(GetLastError());
 THREADENTRY32 entry{sizeof(entry)};LONG result=NO_ERROR;
 if(!Thread32First(snap,&entry)){result=static_cast<LONG>(GetLastError());CloseHandle(snap);return result;}
 for(;;){
  if(entry.th32OwnerProcessID==GetCurrentProcessId() && entry.th32ThreadID!=GetCurrentThreadId()){
   HANDLE h=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_SET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,entry.th32ThreadID);
   if(!h){result=static_cast<LONG>(GetLastError());break;}
   if(SuspendThread(h)==static_cast<DWORD>(-1)){result=static_cast<LONG>(GetLastError());CloseHandle(h);break;}
   opened.push_back(h);result=DetourUpdateThread(h);if(result!=NO_ERROR)break;
  }
  if(!Thread32Next(snap,&entry)){auto error=GetLastError();if(error!=ERROR_NO_MORE_FILES)result=static_cast<LONG>(error);break;}
 }
 CloseHandle(snap);return result;
}
struct Hook{uintptr_t rva;bool se;void** original;void* hook;Byte const* bytes;size_t size;};
static bool ResolveRuntime(HMODULE module,bool& hasCameraEvents){
 using namespace RuntimeResolver;
 auto base=reinterpret_cast<Byte*>(module);auto dos=reinterpret_cast<IMAGE_DOS_HEADER const*>(base);auto nt=reinterpret_cast<IMAGE_NT_HEADERS64 const*>(base+dos->e_lfanew);
 Image image;image.memory={base,nt->OptionalHeader.SizeOfImage};image.pointerBase=reinterpret_cast<uintptr_t>(base);auto section=IMAGE_FIRST_SECTION(nt);
 for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i)image.sections.push_back({section[i].VirtualAddress,std::max(section[i].Misc.VirtualSize,section[i].SizeOfRawData),section[i].Characteristics});
 std::string why;auto begin=GetTickCount64();
 uint32_t assign{},construct{};
 hasCameraEvents=ValidateImage(image,why) && ClassDescriptor(image,"ecl::lua::CameraUpdatingEventParams",assign,construct,why);
 if(hasCameraEvents)return true;
 Resolution result;
 if(!Resolve(image,result,why)){ReportFailure("SE resolver refused before hooks: "+why);return false;}
 RVA_OnUpdate=result.rva[Slot(Id::OnUpdate)];
 RVA_Shutdown=result.rva[Slot(Id::Shutdown)];
 RVA_IncRefs=result.rva[Slot(Id::IncRefs)];
 RVA_DecRefs=result.rva[Slot(Id::DecRefs)];
 RVA_LoadScript=result.rva[Slot(Id::LoadScript)];
 RVA_SetTop=result.rva[Slot(Id::SetTop)];
 RVA_ClientInitialize=result.rva[Slot(Id::ClientInitialize)];
 RVA_PushString=result.rva[Slot(Id::PushString)];
 RVA_PushClosure=result.rva[Slot(Id::PushClosure)];
 RVA_SetTable=result.rva[Slot(Id::SetTable)];
 RVA_CheckStack=result.rva[Slot(Id::CheckStack)];
 RVA_Index=result.rva[Slot(Id::Index)];
 RVA_LifetimeDtor=result.rva[Slot(Id::LifetimeDtor)];
 RVA_AddMember=result.rva[Slot(Id::AddMember)];
 RVA_StaticFloatType=result.rva[Slot(Id::StaticFloatType)];
 RVA_StaticBoolType=result.rva[Slot(Id::StaticBoolType)];
 RVA_StaticUInt16Type=result.rva[Slot(Id::StaticUInt16Type)];
 RVA_StaticEntityType=result.rva[Slot(Id::StaticEntityType)];
 RVA_Alloc=result.rva[Slot(Id::Alloc)];
 RVA_Free=result.rva[Slot(Id::Free)];
 RVA_ValidateMap=result.rva[Slot(Id::ValidateMap)];
 RVA_SerializeRaw=result.rva[Slot(Id::SerializeRaw)];
 RVA_UnserializeRaw=result.rva[Slot(Id::UnserializeRaw)];
 RVA_GetRawObject=result.rva[Slot(Id::GetRawObject)];
 RVA_PushInternalFunction=result.rva[Slot(Id::PushInternalFunction)];
 RVA_DispatchEvent=result.rva[Slot(Id::DispatchEvent)];
 RVA_InheritProperties=result.rva[Slot(Id::InheritProperties)];
 RVA_ProcessDefinitions=result.rva[Slot(Id::ProcessDefinitions)];
 RVA_RegisterClass=result.rva[Slot(Id::RegisterClass)];
 RVA_FixedStringCreate=result.rva[Slot(Id::FixedStringCreate)];
 RVA_FixedStringText=result.rva[Slot(Id::FixedStringText)];
 RVA_FixedStringRelease=result.rva[Slot(Id::FixedStringRelease)];
 RVA_ArgError=result.rva[Slot(Id::ArgError)];
 RVA_LuaError=result.rva[Slot(Id::LuaError)];
 RVA_TypeInitialize=result.rva[Slot(Id::TypeInitialize)];
 RVA_ResizeStructs=result.rva[Slot(Id::ResizeStructs)];
 RVA_LifetimeCtor=result.rva[Slot(Id::LifetimeCtor)];
 SeAbi::FixedStringEvents=result.fixedStringEvents;
 SeAbi::ExtensionLua=result.extensionLua;
 SeAbi::StateLifetimeStack=result.stateLifetimeStack;
 RVA_LifetimeAllocate=result.rva[Slot(Id::LifetimeAllocate)];
 RVA_LifetimePush=result.rva[Slot(Id::LifetimePush)];
 RVA_FinalizeClass=result.rva[Slot(Id::FinalizeClass)];
 RVA_TLSIndex=result.rva[Slot(Id::TLSIndex)];
 RVA_StructRegistry=result.rva[Slot(Id::StructRegistry)];
 RVA_StaticStructs=result.rva[Slot(Id::StaticStructs)];
 RVA_ProfilerCtor=result.rva[Slot(Id::ProfilerCtor)];
 RVA_ProfilerDtor=result.rva[Slot(Id::ProfilerDtor)];
 TLS_ContextOffset=result.contextOffset;
 char message[220];snprintf(message,sizeof(message),"SE instruction patterns resolved; ms=%llu context=%zx registry=%zx statics=%zx finalize=%zx profiler=%zx",GetTickCount64()-begin,TLS_ContextOffset,RVA_StructRegistry,RVA_StaticStructs,RVA_FinalizeClass,RVA_ProfilerCtor);Log(message);return true;
}
static bool ResolveGame(HMODULE module){
 auto base=reinterpret_cast<Byte*>(module);auto dos=reinterpret_cast<IMAGE_DOS_HEADER const*>(base);auto nt=reinterpret_cast<IMAGE_NT_HEADERS64 const*>(base+dos->e_lfanew);
 RuntimeResolver::Image image;image.memory={base,nt->OptionalHeader.SizeOfImage};image.pointerBase=reinterpret_cast<uintptr_t>(base);auto section=IMAGE_FIRST_SECTION(nt);
 for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i)image.sections.push_back({section[i].VirtualAddress,std::max(section[i].Misc.VirtualSize,section[i].SizeOfRawData),section[i].Characteristics});
 GameResolver::Resolution result;std::string why;auto begin=GetTickCount64();
 if(!GameResolver::Resolve(image,result,why)){ReportFailure("game function mapping refused before hooks: "+why);return false;}
 if(!InputBindingResolver::Resolve(image,InputBindings::addresses,why)){
  ReportStartup(("WARNING: automatic movement binding setup unavailable: "+why).c_str());InputBindings::addresses={};
 }
 if(!RotationCursorResolver::Resolve(image,RotationCursor::addresses,why)){
  ReportStartup(("WARNING: rotation cursor unavailable: "+why).c_str());RotationCursor::addresses={};
 }
 RVA_UpdateCamera=result.rva[static_cast<size_t>(GameResolver::Id::UpdateCamera)];
 RVA_UpdatePitch=result.rva[static_cast<size_t>(GameResolver::Id::UpdatePitch)];
 RVA_UpdateZoom=result.rva[static_cast<size_t>(GameResolver::Id::UpdateZoom)];
 RVA_Definition=result.rva[static_cast<size_t>(GameResolver::Id::Definition)];
 RVA_Pitch=result.rva[static_cast<size_t>(GameResolver::Id::Pitch)];
 RVA_Input=result.rva[static_cast<size_t>(GameResolver::Id::Input)];
 RVA_ResetZoom=result.rva[static_cast<size_t>(GameResolver::Id::ResetZoom)];
 RVA_KeyboardGate=result.rva[static_cast<size_t>(GameResolver::Id::KeyboardGate)];
 char message[180];snprintf(message,sizeof(message),"game function patterns resolved; ms=%llu camera=%zx definition=%zx keyboard=%zx",GetTickCount64()-begin,RVA_UpdateCamera,RVA_Definition,RVA_KeyboardGate);Log(message);return true;
}
static bool LoadBootstrap(HMODULE module){
 HRSRC resource=FindResourceW(module,MAKEINTRESOURCEW(MAZZLE_BOOTSTRAP_RESOURCE),MAKEINTRESOURCEW(10));
 if(!resource)return false;
 DWORD size=SizeofResource(module,resource);HGLOBAL loaded=LoadResource(module,resource);
 auto bytes=loaded?static_cast<char const*>(LockResource(loaded)):nullptr;
 if(!bytes||!size||std::memchr(bytes,0,size))return false;
 companion.assign(bytes,size);return true;
}
static DWORD WINAPI Start(void* instance){
 bool logAvailable=OpenLog();Log("standalone camera/input bridge starts");
 RuntimeModule::Finder finder;RuntimeModule::Candidate candidate;bool ambiguous=false;
 for(unsigned i=0;i<600 && !candidate.module;++i){candidate=finder.Find(ambiguous);if(ambiguous){ReportFailure("multiple standalone Script Extender modules found; no hooks installed");return 2;}if(!candidate.module)Sleep(100);}
 if(!candidate.module){ReportFailure("could not find a loaded Script Extender module within 60 seconds; checked the standard DLL name and SE version metadata");return 2;}
 HMODULE se=candidate.module;
 ReportStartup("STARTING: Script Extender found; checking compatibility.");
 if(!logAvailable)ReportStartup("WARNING: could not open the LocalAppData log; console reporting remains available.");
 if(!LoadBootstrap(static_cast<HMODULE>(instance))){ReportFailure("embedded Lua bootstrap missing or invalid; no hooks installed");return 1;}
 if(candidate.path.empty()){ReportFailure("cannot locate the loaded Script Extender DLL");return 3;}
 bool hasCameraEvents=false;
 if(!ResolveRuntime(se,hasCameraEvents))return 3;
 if(hasCameraEvents){ReportStartup("SKIPPED: Script Extender provides native camera events; no bridge hooks or bindings installed.");return 0;}
 auto game=GetModuleHandleW(nullptr);
 if(!ResolveGame(game))return 4;
 seBase=reinterpret_cast<Byte*>(se);gameBase=reinterpret_cast<Byte*>(game);
 Hook hooks[]={{RVA_TypeInitialize,true,reinterpret_cast<void**>(&NativePayload::nextTypeInitialize),reinterpret_cast<void*>(NativePayload::InitializeTypes),nullptr,0},{RVA_ResizeStructs,true,reinterpret_cast<void**>(&NativePayload::nextResize),reinterpret_cast<void*>(NativePayload::ResizeStatics),nullptr,0},{RVA_OnUpdate,true,reinterpret_cast<void**>(&nextUpdate),reinterpret_cast<void*>(Update),nullptr,0},{RVA_Shutdown,true,reinterpret_cast<void**>(&nextShutdown),reinterpret_cast<void*>(Shutdown),nullptr,0},{RVA_ClientInitialize,true,reinterpret_cast<void**>(&nextInitialize),reinterpret_cast<void*>(InitializeState),nullptr,0},{RVA_UpdateCamera,false,reinterpret_cast<void**>(&nextCamera),reinterpret_cast<void*>(Camera),Bytes_UpdateCamera,sizeof(Bytes_UpdateCamera)},{RVA_UpdatePitch,false,reinterpret_cast<void**>(&nextUpdatePitch),reinterpret_cast<void*>(UpdatePitch),Bytes_UpdatePitch,sizeof(Bytes_UpdatePitch)},{RVA_UpdateZoom,false,reinterpret_cast<void**>(&nextZoom),reinterpret_cast<void*>(Zoom),Bytes_UpdateZoom,sizeof(Bytes_UpdateZoom)},{RVA_Pitch,false,reinterpret_cast<void**>(&nextPitch),reinterpret_cast<void*>(Pitch),Bytes_Pitch,sizeof(Bytes_Pitch)},{RVA_Input,false,reinterpret_cast<void**>(&nextInput),reinterpret_cast<void*>(Input),Bytes_Input,sizeof(Bytes_Input)},{RVA_ResetZoom,false,reinterpret_cast<void**>(&nextReset),reinterpret_cast<void*>(ResetZoom),Bytes_ResetZoom,sizeof(Bytes_ResetZoom)}};
 for(auto& hook:hooks){auto target=(hook.se?seBase:gameBase)+hook.rva;if(hook.bytes && memcmp(target,hook.bytes,hook.size)){ReportFailure("hook prologue mismatch; no hooks installed");return 5;}*hook.original=target;}
 CallPatch keyboard,pathCall,remapCall;auto site=gameBase+RVA_KeyboardGate;
 int32_t modeOffset;memcpy(&modeOffset,site+2,4);KeyboardMovementControllerMode=site+7+modeOffset;
 bool keyboardPrepared=keyboard.Prepare(site,reinterpret_cast<void*>(&KeyboardMovementModeBridge),true);
 auto const& bindings=InputBindings::addresses;bool bindingsPrepared=false;
 if(bindings.reload){
  InputBindings::reload=reinterpret_cast<InputBindings::ReloadFn>(gameBase+bindings.reload);
  InputBindings::nextPath=reinterpret_cast<InputBindings::PathFn>(gameBase+bindings.pathTarget);
  InputBindings::nextRemap=reinterpret_cast<InputBindings::RemapFn>(gameBase+bindings.remapTarget);
  bindingsPrepared=pathCall.Prepare(gameBase+bindings.pathCall,reinterpret_cast<void*>(InputBindings::CapturePath)) && remapCall.Prepare(gameBase+bindings.remapCall,reinterpret_cast<void*>(InputBindings::AfterRemap));
 }
 CallPatch cursorCalls[6];bool cursorPrepared=false;
 auto const& cursor=RotationCursor::addresses;
 if(cursor.on && RotationCursor::Initialize() && *reinterpret_cast<void**>(gameBase+cursor.pollImport)==reinterpret_cast<void*>(RotationCursor::nextPoll)){
  RotationCursor::nextOn=reinterpret_cast<RotationCursor::RotationFn>(gameBase+cursor.on);
  RotationCursor::nextOff=reinterpret_cast<RotationCursor::RotationFn>(gameBase+cursor.off);
  void* destinations[]={reinterpret_cast<void*>(RotationCursor::On),reinterpret_cast<void*>(RotationCursor::Off),reinterpret_cast<void*>(RotationCursor::Off),reinterpret_cast<void*>(RotationCursor::Off),reinterpret_cast<void*>(RotationCursor::Poll),reinterpret_cast<void*>(RotationCursor::Virtual)};
  cursorPrepared=true;
  for(size_t i=0;i<std::size(cursorCalls);++i)if(!cursorCalls[i].Prepare(gameBase+cursor.sites[i],destinations[i],false,i<4?5:6)){cursorPrepared=false;break;}
 }
 LONG result=DetourTransactionBegin();std::vector<HANDLE> opened;
 if(result==NO_ERROR)result=EnlistThreads(opened);
 bool haveRegistration=result==NO_ERROR;NativePayload::RegistrationState registration{};
 if(haveRegistration)registration=NativePayload::CaptureRegistrationState();
 bool lateRegistration=haveRegistration && registration.statics!=0;
 if(lateRegistration)result=ERROR_INVALID_STATE;
 for(auto& hook:hooks)if(result==NO_ERROR)result=DetourAttach(hook.original,hook.hook);
 bool keyboardInstalled=false,bindingsInstalled=false,cursorInstalled=false;
 if(result==NO_ERROR){
  if(keyboardPrepared){keyboardInstalled=keyboard.Apply();if(!keyboardInstalled)keyboard.Rollback();}
  if(bindingsPrepared){bindingsInstalled=pathCall.Apply() && remapCall.Apply();if(!bindingsInstalled){remapCall.Rollback();pathCall.Rollback();}}
  if(cursorPrepared){cursorInstalled=true;for(auto& patch:cursorCalls)if(!patch.Apply()){cursorInstalled=false;break;}if(!cursorInstalled)for(auto& patch:cursorCalls)patch.Rollback();}
  result=DetourTransactionCommit();
 }else DetourTransactionAbort();
 if(result!=NO_ERROR){for(auto& patch:cursorCalls)patch.Rollback();remapCall.Rollback();pathCall.Rollback();keyboard.Rollback();}
 RotationCursor::available=result==NO_ERROR && cursorInstalled;
 keyboardAvailable.store(result==NO_ERROR && keyboardInstalled);InputBindings::available=result==NO_ERROR && bindingsInstalled;
 // Our extra suspension keeps raw CALL patches protected even when Detours
 // resumes its own enlisted threads on commit or abort.
 for(HANDLE h:opened){ResumeThread(h);CloseHandle(h);}
 if(haveRegistration)NativePayload::LogRegistrationState("before hook installation",registration);
 keyboard.Discard();pathCall.Discard();remapCall.Discard();for(auto& patch:cursorCalls)patch.Discard();
 char message[160];snprintf(message,sizeof(message),"hooks result=%ld keyboard=%d bindings=%d; default override false",result,keyboardAvailable.load(),InputBindings::available);Log(message);
 if(result!=NO_ERROR)ReportFailure(lateRegistration?"SE type registration already completed before native loader initialization; no hooks installed":"hook installation failed with Windows error "+std::to_string(result));
 else{if(!RotationCursor::available)ReportStartup("WARNING: rotation cursor hooks unavailable; mouse confinement is disabled.");else Log("rotation cursor main-thread hooks installed");if(!keyboardAvailable.load())ReportStartup("WARNING: keyboard movement hook failed; WASD is unavailable.");if(!InputBindings::available)ReportStartup("WARNING: movement binding hooks failed; automatic WASD setup is unavailable.");}
 return result;
}
// Native Mod Loader calls Init after LoadLibrary returns, before continuing
// the game's CRT initialization. Finish installing type hooks at that boundary.
extern "C" __declspec(dllexport) void Init(){if(!startupRequested.test_and_set())Start(bridgeInstance);}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){bridgeInstance=instance;DisableThreadLibraryCalls(instance);}return TRUE;}







#include <windows.h>
#include <tlhelp32.h>
#include <bcrypt.h>
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
using Byte=unsigned char;
#include "addresses.h"
#pragma comment(lib,"bcrypt.lib")
static Byte* seBase; static Byte* gameBase;
static std::atomic<void*> clientExt{};
static std::atomic<unsigned> generation{};
static std::atomic<void*> installedState{}; static std::atomic<bool> enabled{false};
static FILE* logFile;static SRWLOCK logLock=SRWLOCK_INIT;
static std::atomic<unsigned long long> calls[10]{},errors{},prevented{},overrides{};
// Retained statistics slot for previous tools; native pin accepts all threads, zero means none dropped.
static constexpr unsigned long long wrongThread=0;
static std::string companion;
extern "C" { std::atomic<uint8_t> KeyboardMovementOverride{0}; uint8_t const* KeyboardMovementControllerMode=nullptr; void KeyboardMovementModeBridge(); }
static_assert(sizeof(KeyboardMovementOverride)==1);
static bool keyboardAvailable;
static void Log(char const* message){AcquireSRWLockExclusive(&logLock);if(logFile){fprintf(logFile,"%llu %s\n",GetTickCount64(),message);fflush(logFile);}ReleaseSRWLockExclusive(&logLock);}
template<class T>T& Field(void* p,size_t offset){return *reinterpret_cast<T*>(static_cast<Byte*>(p)+offset);}
template<class F>F Se(uintptr_t rva){return reinterpret_cast<F>(seBase+rva);}
struct SeString{union{char const* ptr;char inlineData[16];};uint32_t size,capacity;};
struct OptionalInt{int value;bool hasValue;Byte padding[3];};
static SeString Borrow(char const* text){SeString s{};s.size=static_cast<uint32_t>(strlen(text));if(s.size<=15){memcpy(s.inlineData,text,s.size+1);s.capacity=15;}else{s.ptr=text;s.capacity=s.size;}return s;}
#include "lua_value.h"
#include "native_encoding.h"
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
static int Statistics(void* L){for(auto& count:calls)PushInt(L,static_cast<int64_t>(count.load()));PushInt(L,static_cast<int64_t>(errors.load()));PushInt(L,static_cast<int64_t>(wrongThread));PushInt(L,generation.load());PushInt(L,static_cast<int64_t>(prevented.load()));PushInt(L,static_cast<int64_t>(overrides.load()));return 15;}
static void Bind(void* L,char const* name,int(*fn)(void*)){Se<void(*)(void*,char const*)>(RVA_PushString)(L,name);Se<void(*)(void*,int(*)(void*),int)>(RVA_PushClosure)(L,fn,0);Se<void(*)(void*,int)>(RVA_SetTable)(L,-3);}
#include "native_payload.h"
using InitializeFn=void(*)(void*);static InitializeFn nextInitialize;
static void InitializeState(void* state){++NativePayload::vmCount;nextInitialize(state);installedState=state;enabled=false;void* L=Field<void*>(state,SeAbi::StateLua);int top=Top(L);if(!Se<int(*)(void*,int)>(RVA_CheckStack)(L,64)){Log("cannot reserve Lua stack");return;}auto ret=Load(state,"local b={}; Ext._Internal.MazzleNativeBridge=b; return b","NativeBridgeBindings");if(!ret.hasValue || ret.value!=1){SetTop(L,top);Log("binding table creation failed");return;}Bind(L,"SetKeyboardMovementOverride",KeyboardSet);Bind(L,"GetKeyboardMovementOverride",KeyboardGet);Bind(L,"GetCameraDefinition",NativePayload::Definition);Bind(L,"NativeStatistics",Statistics);Bind(L,"NativePayloadStatus",NativePayload::Status);Bind(L,"NativePayloadValidationStatus",NativePayload::ValidationStatus);SetTop(L,top);ret=Load(state,companion.c_str(),"NativeBridgeCompanion");if(ret.hasValue && ret.value==1){enabled=NativePayload::ready;Log("all native camera events registered before mod bootstrap");}else Log("companion initialization failed; native fallback retained");SetTop(L,top);}
using UpdateFn=void(*)(void*,void const*);static UpdateFn nextUpdate;
static void Update(void* ext,void const* time){clientExt.store(ext);nextUpdate(ext,time);}
using ShutdownFn=void(*)(void*);static ShutdownFn nextShutdown;
static void Shutdown(void* ext){enabled=false;installedState=nullptr;clientExt.store(nullptr);KeyboardMovementOverride.store(0);++generation;Log("client VM shutdown: dispatch ref invalidated; keyboard override false");nextShutdown(ext);}
struct DispatchResult{bool successful=false;float value=0;bool hasNumber=false;bool prevented=false;uint16_t inputResult=0;};
static DispatchResult Dispatch(int event,void* camera,float dt,uint64_t entity=0,void* input=nullptr,double original=0,bool ignore=false,bool select=false){
 DispatchResult result;void* ext=clientExt.load();if(!ext)return result;
 auto tls=reinterpret_cast<Byte**>(__readgsqword(0x58));auto tlsIndex=*reinterpret_cast<DWORD*>(seBase+RVA_TLSIndex);int& context=*reinterpret_cast<int*>(tls[tlsIndex]+TLS_ContextOffset);int priorContext=context;context=2;
 Se<void(*)(void*)>(RVA_IncRefs)(ext);void* state=Field<void*>(ext,SeAbi::ExtensionLua);
 if(enabled.load() && !Field<bool>(ext,SeAbi::ExtensionResetPending) && state && state==installedState.load()){
  void* L=Field<void*>(state,SeAbi::StateLua);int top=Top(L);
  if(Se<int(*)(void*,int)>(RVA_CheckStack)(L,64)){
   alignas(8) Byte lifetime[SeAbi::PinBytes]{};
   InitializeLifetimePin(lifetime,L,static_cast<Byte*>(state)+SeAbi::StateLifetimeStack);
   auto native=NativePayload::Dispatch(static_cast<unsigned>(event),state,L,camera,dt,entity,input,static_cast<float>(original),ignore,select,*Field<uint64_t*>(lifetime,SeAbi::PinCurrentReference));++calls[event];
   result.successful=native.status==0;result.prevented=native.status==2;result.value=native.pitch;result.hasNumber=event==5;result.inputResult=native.input;
   if(native.status==1){++errors;Log("native event dispatch failed; engine fallback retained");}
   SetTop(L,top);Se<void(*)(void*)>(RVA_LifetimeDtor)(lifetime);
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
static float Pitch(void* camera,bool ignore,bool select){float pitch=nextPitch(camera,ignore,select);auto result=Dispatch(5,camera,0,0,nullptr,pitch,ignore,select);if(result.successful && result.hasNumber && std::isfinite(result.value) && std::isfinite(static_cast<float>(result.value))){++overrides;return static_cast<float>(result.value);}return pitch;}
using InputFn=uint16_t*(*)(void*,uint16_t*,void*,void*);static InputFn nextInput;
static uint16_t* Input(void* self,uint16_t* result,void* view,void* input){auto camera=Field<void*>(view,0x40);auto event=Dispatch(6,camera,0,0,input);if(event.prevented){*result=event.inputResult;++prevented;return result;}auto ret=nextInput(self,result,view,input);Dispatch(7,Field<void*>(view,0x40),0,0,input,*ret);return ret;}
using ResetZoomFn=void(*)(void*);static ResetZoomFn nextReset;
static void ResetZoom(void* view){auto camera=Field<void*>(view,0x20);Dispatch(8,camera,0);nextReset(view);Dispatch(9,Field<void*>(view,0x20),0);}
static std::wstring HashFile(wchar_t const* path){HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);if(file==INVALID_HANDLE_VALUE)return {};BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;Byte digest[32];if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0){CloseHandle(file);return {};}if(BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)<0){BCryptCloseAlgorithmProvider(alg,0);CloseHandle(file);return {};}Byte buf[65536];DWORD count;bool ok=true;for(;;){if(!ReadFile(file,buf,sizeof(buf),&count,nullptr)){ok=false;break;}if(!count)break;if(BCryptHashData(hash,buf,count,0)<0){ok=false;break;}}if(BCryptFinishHash(hash,digest,sizeof(digest),0)<0)ok=false;BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(alg,0);CloseHandle(file);if(!ok)return {};wchar_t hex[65];for(unsigned i=0;i<32;++i)swprintf_s(hex+i*2,3,L"%02x",digest[i]);return hex;}
static void* AllocateRelay(uintptr_t site){SYSTEM_INFO info;GetSystemInfo(&info);auto granularity=static_cast<uintptr_t>(info.dwAllocationGranularity);auto lower=site>0x7fff0000ull?site-0x7fff0000ull:granularity;auto upper=site+0x7fff0000ull;auto address=(lower+granularity-1)&~(granularity-1);while(address<upper){MEMORY_BASIC_INFORMATION region;if(!VirtualQuery(reinterpret_cast<void*>(address),&region,sizeof(region)))break;auto end=reinterpret_cast<uintptr_t>(region.BaseAddress)+region.RegionSize;if(region.State==MEM_FREE && end-address>=info.dwPageSize){if(auto relay=VirtualAlloc(reinterpret_cast<void*>(address),info.dwPageSize,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE))return relay;}if(end<=address)break;address=(end+granularity-1)&~(granularity-1);}return nullptr;}
static bool InstallKeyboard(){auto site=gameBase+RVA_KeyboardGate;auto relay=static_cast<Byte*>(AllocateRelay(reinterpret_cast<uintptr_t>(site)));if(!relay)return false;int32_t modeOffset;memcpy(&modeOffset,site+2,4);KeyboardMovementControllerMode=site+7+modeOffset;Byte jump[]={0xff,0x25,0,0,0,0};memcpy(relay,jump,6);auto bridge=&KeyboardMovementModeBridge;memcpy(relay+6,&bridge,8);DWORD old;if(!VirtualProtect(relay,14,PAGE_EXECUTE_READ,&old))return false;FlushInstructionCache(GetCurrentProcess(),relay,14);auto displacement=reinterpret_cast<intptr_t>(relay)-reinterpret_cast<intptr_t>(site+5);if(displacement<INT32_MIN || displacement>INT32_MAX)return false;Byte replacement[]={0xe8,0,0,0,0,0x84,0xc0};int32_t rel32=static_cast<int32_t>(displacement);memcpy(replacement+1,&rel32,4);if(!VirtualProtect(site,7,PAGE_EXECUTE_READWRITE,&old))return false;memcpy(site,replacement,7);DWORD ignored;VirtualProtect(site,7,old,&ignored);FlushInstructionCache(GetCurrentProcess(),site,7);return true;}
static bool EnlistThreads(std::vector<HANDLE>& opened){HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);if(snap==INVALID_HANDLE_VALUE)return false;THREADENTRY32 entry{sizeof(entry)};bool ok=true;if(Thread32First(snap,&entry))do{if(entry.th32OwnerProcessID==GetCurrentProcessId() && entry.th32ThreadID!=GetCurrentThreadId()){HANDLE h=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_SET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,entry.th32ThreadID);if(!h){ok=false;break;}opened.push_back(h);if(DetourUpdateThread(h)!=NO_ERROR){ok=false;break;}}}while(Thread32Next(snap,&entry));CloseHandle(snap);return ok;}
struct Hook{uintptr_t rva;bool se;void** original;void* hook;Byte const* bytes;size_t size;};
static DWORD WINAPI Start(void* instance){logFile=_wfsopen(LR"(C:\Users\mazzin\AppData\Local\Larian Studios\Baldur's Gate 3\Script Extender\MazzleNativeCameraBridgeFull.log)",L"w",_SH_DENYNO);Log("full experimental standalone camera/input bridge starts");wchar_t path[32768];GetModuleFileNameW(static_cast<HMODULE>(instance),path,32768);std::wstring companionPath(path);companionPath=companionPath.substr(0,companionPath.find_last_of(L"\\/")+1)+L"bridge.lua";std::ifstream file(companionPath);companion.assign(std::istreambuf_iterator<char>(file),{});if(companion.empty()){Log("bridge.lua missing; no hooks installed");return 1;}HMODULE se=nullptr;for(unsigned i=0;i<600 && !se;++i){se=GetModuleHandleW(L"BG3ScriptExtender.dll");if(!se)Sleep(100);}if(!se){Log("SE missing");return 2;}GetModuleFileNameW(se,path,32768);if(HashFile(path)!=SE_HASH){Log("SE hash mismatch; no hooks installed");return 3;}GetModuleFileNameW(nullptr,path,32768);if(HashFile(path)!=GAME_HASH){Log("game hash mismatch; no hooks installed");return 4;}seBase=reinterpret_cast<Byte*>(se);gameBase=reinterpret_cast<Byte*>(GetModuleHandleW(nullptr));
 Hook hooks[]={{RVA_TypeInitialize,true,reinterpret_cast<void**>(&NativePayload::nextTypeInitialize),reinterpret_cast<void*>(NativePayload::InitializeTypes),Bytes_TypeInitialize,sizeof(Bytes_TypeInitialize)},{RVA_ResizeStructs,true,reinterpret_cast<void**>(&NativePayload::nextResize),reinterpret_cast<void*>(NativePayload::ResizeStatics),Bytes_ResizeStructs,sizeof(Bytes_ResizeStructs)},{RVA_OnUpdate,true,reinterpret_cast<void**>(&nextUpdate),reinterpret_cast<void*>(Update),Bytes_OnUpdate,sizeof(Bytes_OnUpdate)},{RVA_Shutdown,true,reinterpret_cast<void**>(&nextShutdown),reinterpret_cast<void*>(Shutdown),Bytes_Shutdown,sizeof(Bytes_Shutdown)},{RVA_ClientInitialize,true,reinterpret_cast<void**>(&nextInitialize),reinterpret_cast<void*>(InitializeState),Bytes_ClientInitialize,sizeof(Bytes_ClientInitialize)},{RVA_UpdateCamera,false,reinterpret_cast<void**>(&nextCamera),reinterpret_cast<void*>(Camera),Bytes_UpdateCamera,sizeof(Bytes_UpdateCamera)},{RVA_UpdatePitch,false,reinterpret_cast<void**>(&nextUpdatePitch),reinterpret_cast<void*>(UpdatePitch),Bytes_UpdatePitch,sizeof(Bytes_UpdatePitch)},{RVA_UpdateZoom,false,reinterpret_cast<void**>(&nextZoom),reinterpret_cast<void*>(Zoom),Bytes_UpdateZoom,sizeof(Bytes_UpdateZoom)},{RVA_Pitch,false,reinterpret_cast<void**>(&nextPitch),reinterpret_cast<void*>(Pitch),Bytes_Pitch,sizeof(Bytes_Pitch)},{RVA_Input,false,reinterpret_cast<void**>(&nextInput),reinterpret_cast<void*>(Input),Bytes_Input,sizeof(Bytes_Input)},{RVA_ResetZoom,false,reinterpret_cast<void**>(&nextReset),reinterpret_cast<void*>(ResetZoom),Bytes_ResetZoom,sizeof(Bytes_ResetZoom)}};
 for(auto& hook:hooks){auto target=(hook.se?seBase:gameBase)+hook.rva;if(hook.bytes && memcmp(target,hook.bytes,hook.size)){Log("hook prologue mismatch; no hooks installed");return 5;}*hook.original=target;}if(memcmp(gameBase+RVA_KeyboardGate,Bytes_KeyboardGate,sizeof(Bytes_KeyboardGate)) || memcmp(gameBase+RVA_Definition,Bytes_Definition,sizeof(Bytes_Definition))){Log("gate/getter prologue mismatch");return 6;}
 LONG result=DetourTransactionBegin();std::vector<HANDLE> opened;if(result==NO_ERROR && !EnlistThreads(opened))result=ERROR_ACCESS_DENIED;for(auto& hook:hooks)if(result==NO_ERROR)result=DetourAttach(hook.original,hook.hook);if(result==NO_ERROR)result=DetourTransactionCommit();else DetourTransactionAbort();for(HANDLE h:opened)CloseHandle(h);if(result==NO_ERROR)keyboardAvailable=InstallKeyboard();char message[140];snprintf(message,sizeof(message),"hooks result=%ld keyboard=%d; default override false",result,keyboardAvailable);Log(message);return result;
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){DisableThreadLibraryCalls(instance);HANDLE thread=CreateThread(nullptr,0,Start,instance,0,nullptr);if(thread)CloseHandle(thread);}return TRUE;}







#pragma once
#include <filesystem>
#include "input_binding_config.h"
#include "input_binding_resolver.h"

namespace InputBindings {
using PathFn=uint64_t*(*)(uint64_t*,uint32_t*,void*);
using RemapFn=void(*)(int64_t);
using ReloadFn=void(*)(int64_t,char);
static PathFn nextPath;static RemapFn nextRemap;static ReloadFn reload;
static InputBindingResolver::Resolution addresses;
static bool available;
static SRWLOCK lock=SRWLOCK_INIT;
static std::wstring path;
static std::atomic<bool> active{false};
struct Exclusive {Exclusive(){AcquireSRWLockExclusive(&lock);}~Exclusive(){ReleaseSRWLockExclusive(&lock);}};
static uint64_t* CapturePath(uint64_t* source,uint32_t* second,void* third){
 auto result=nextPath(source,second,third);
 auto text=result?reinterpret_cast<char const*>(*result):nullptr;
 if(text){
  // Capture the actual selected profile; do not assume the Public profile or
  // a user's drive. Reload also visits p2-p4 and controller config files.
  auto filename=strrchr(text,'/');auto windowsSeparator=strrchr(text,'\\');
  if(windowsSeparator && (!filename || windowsSeparator>filename))filename=windowsSeparator;
  if(filename && !_stricmp(filename+1,"inputconfig_p1.json")){
   int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text,-1,nullptr,0);
   if(count>1){std::wstring selected(static_cast<size_t>(count),L'\0');if(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text,-1,selected.data(),count)){
    selected.resize(static_cast<size_t>(count)-1);Exclusive guard;if(path!=selected){path=std::move(selected);Log((std::string("active player input config path: ")+text).c_str());}
   }}
  }
 }
 return result;
}
static bool Write(std::wstring const& selected,std::string const& contents,std::string& why){
 auto temporary=selected+L".camera-bridge-"+std::to_wstring(GetCurrentProcessId())+L".tmp";
 {std::ofstream output(std::filesystem::path(temporary),std::ios::binary|std::ios::trunc);
  if(!output){why="cannot open temporary input config";return false;}
  output.write(contents.data(),static_cast<std::streamsize>(contents.size()));output.close();
  if(!output){DeleteFileW(temporary.c_str());why="cannot write input config";return false;}
 }
 DWORD error{};
 for(unsigned attempt=0;attempt<20;++attempt){
  if(MoveFileExW(temporary.c_str(),selected.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return true;
  error=GetLastError();if(error!=ERROR_SHARING_VIOLATION && error!=ERROR_ACCESS_DENIED)break;Sleep(25);
 }
 DeleteFileW(temporary.c_str());why="cannot replace input config (Windows error "+std::to_string(error)+")";return false;
}
static bool Sync(){
 if(!available)return false;
 bool missing;{Exclusive guard;missing=path.empty();}
 if(missing)reload(0,0);
 bool changed=false;std::string why;
 {
  Exclusive guard;
  if(path.empty())why="game did not provide the active player input config path";
  else{
   std::ifstream input(std::filesystem::path(path),std::ios::binary);
   if(!input)why="cannot open the active player input config";
   else{
    std::string contents{std::istreambuf_iterator<char>(input),{}};input.close();
    rapidjson::Document document;document.Parse(contents.data(),contents.size());
    if(InputBindingConfig::Build(document,changed,why) && changed)Write(path,InputBindingConfig::Serialize(document),why);
   }
  }
 }
 if(!why.empty()){ReportStartup(("WARNING: movement binding setup failed: "+why).c_str());return false;}
 if(changed)reload(0,0);
 active.store(true,std::memory_order_release);Log(changed?"movement bindings synchronized and reloaded":"movement bindings already synchronized");return true;
}
static void AfterRemap(int64_t menu){Log("input keybinding menu commit entered");nextRemap(menu);Log("input keybinding menu commit returned");if(active.load(std::memory_order_acquire) && !Sync())KeyboardMovementOverride.store(0);}
static int Synchronize(void* L){PushBool(L,installedState.load()!=nullptr && Sync());return 1;}
static void Shutdown(){active.store(false,std::memory_order_release);}
}

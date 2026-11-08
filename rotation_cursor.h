#pragma once
#include <SDL.h>
#include "rotation_cursor_resolver.h"

// Rotation notifications arrive on the game thread. SDL mode changes and the
// virtual cursor run on the main thread, at the original WASD call sites.
namespace RotationCursor {
static RotationCursorResolver::Resolution addresses;
static bool available=false;
static SRWLOCK lock=SRWLOCK_INIT;
struct Guard { Guard(){AcquireSRWLockExclusive(&lock);} ~Guard(){ReleaseSRWLockExclusive(&lock);} };
static std::atomic<bool> work{false},virtualWork{false};
static bool policy=false,rotating=false,owned=false,cancel=false,failed=false;
static SDL_Window* window{};
static int savedX{},savedY{},width{};
static float virtualX{},virtualY{};
enum class VirtualState {Normal,Pending,Override,Restore};
static VirtualState virtualState=VirtualState::Normal;
static std::string priorCenter,priorVisible;
static bool hadCenter=false,hadVisible=false;
static decltype(&SDL_SetRelativeMouseMode) relative{};
static decltype(&SDL_GetRelativeMouseMode) getRelative{};
static decltype(&SDL_GetMouseState) mouse{};
static decltype(&SDL_GetKeyboardFocus) keyboardFocus{};
static decltype(&SDL_GetWindowSize) size{};
static decltype(&SDL_WarpMouseInWindow) warp{};
static decltype(&SDL_SetHint) hint{};
static decltype(&SDL_GetHint) getHint{};
static decltype(&SDL_ResetHint) resetHint{};
static decltype(&SDL_PushEvent) push{};
static decltype(&SDL_GetWindowID) windowId{};
static decltype(&SDL_PollEvent) nextPoll{};
using RotationFn=void(*)(void*,int);
static RotationFn nextOn{},nextOff{};
static bool Initialize(){
 auto module=GetModuleHandleW(L"SDL2.dll");if(!module)return false;
#define CURSOR_EXPORT(variable,name) variable=reinterpret_cast<decltype(variable)>(GetProcAddress(module,#name));if(!variable)return false
 CURSOR_EXPORT(relative,SDL_SetRelativeMouseMode);CURSOR_EXPORT(getRelative,SDL_GetRelativeMouseMode);
 CURSOR_EXPORT(mouse,SDL_GetMouseState);CURSOR_EXPORT(keyboardFocus,SDL_GetKeyboardFocus);
 CURSOR_EXPORT(size,SDL_GetWindowSize);CURSOR_EXPORT(warp,SDL_WarpMouseInWindow);
 CURSOR_EXPORT(hint,SDL_SetHint);CURSOR_EXPORT(getHint,SDL_GetHint);CURSOR_EXPORT(resetHint,SDL_ResetHint);
 CURSOR_EXPORT(push,SDL_PushEvent);CURSOR_EXPORT(windowId,SDL_GetWindowID);CURSOR_EXPORT(nextPoll,SDL_PollEvent);
#undef CURSOR_EXPORT
 return true;
}
static void Request(bool value){
 Guard guard;if(policy==value)return;policy=value;
 if(!value){cancel=true;work.store(true,std::memory_order_release);}
 else if(rotating)work.store(true,std::memory_order_release);
}
static int Set(void* L){
 if(At(L,1)->tag!=1)return Se<int(*)(void*,int,char const*)>(RVA_ArgError)(L,1,"boolean expected");
 if(available)Request(Boolean(L,1));PushBool(L,available);return 1;
}
static void On(void* self,int command){
 {
  Guard guard;
  if(!rotating){
   // Never replace the original anchor with SDL's relative-mode position.
   if(!owned){mouse(&savedX,&savedY);window=keyboardFocus();}
   rotating=true;failed=false;if(policy)work.store(true,std::memory_order_release);
  }
 }
 nextOn(self,command);
}
static void Off(void* self,int command){
 {Guard guard;rotating=false;failed=false;if(owned)work.store(true,std::memory_order_release);}
 nextOff(self,command);
}
static void Motion(){
 SDL_Event event{};event.type=SDL_MOUSEMOTION;event.motion.windowID=windowId(window);
 event.motion.x=savedX;event.motion.y=savedY;push(&event);
}
static void RestoreHints(){
 if(hadCenter)hint(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER,priorCenter.c_str());else resetHint(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER);
 if(hadVisible)hint(SDL_HINT_MOUSE_RELATIVE_CURSOR_VISIBLE,priorVisible.c_str());else resetHint(SDL_HINT_MOUSE_RELATIVE_CURSOR_VISIBLE);
}
static void Release(){
 if(!owned)return;
 // WASD's ordering avoids a synthetic rotation delta on release.
 warp(window,savedX,savedY);relative(SDL_FALSE);RestoreHints();
 owned=false;virtualState=VirtualState::Restore;virtualWork.store(true,std::memory_order_release);Motion();
}
static void Update(){
 if(!work.load(std::memory_order_acquire))return;
 Guard guard;
 if(cancel){Release();cancel=false;failed=false;}
 if(owned && keyboardFocus()!=window){Release();rotating=false;}
 if(!policy || !rotating){Release();work.store(false,std::memory_order_release);return;}
 if(!owned && !failed){
  if(!window || keyboardFocus()!=window || getRelative()!=SDL_FALSE){failed=true;work.store(false,std::memory_order_release);return;}
  auto center=getHint(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER);hadCenter=center!=nullptr;priorCenter=center?center:"";
  auto visible=getHint(SDL_HINT_MOUSE_RELATIVE_CURSOR_VISIBLE);hadVisible=visible!=nullptr;priorVisible=visible?visible:"";
  hint(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER,"0");hint(SDL_HINT_MOUSE_RELATIVE_CURSOR_VISIBLE,"0");
  if(relative(SDL_TRUE)!=0){RestoreHints();failed=true;work.store(false,std::memory_order_release);Log("rotation cursor relative mode unavailable; request left inactive");return;}
  int height{};size(window,&width,&height);owned=true;virtualX=static_cast<float>(savedX);virtualY=static_cast<float>(savedY);virtualState=VirtualState::Pending;virtualWork.store(true,std::memory_order_release);
 }
 // Preserve WASD's six-pixel drag threshold before suppressing UI hover.
 if(owned && virtualState==VirtualState::Pending){
  double dx=static_cast<double>(virtualX)-savedX,dy=static_cast<double>(virtualY)-savedY;
  if(dx*dx+dy*dy>36){virtualState=VirtualState::Override;Motion();}
 }
}
static int Poll(SDL_Event* event){
 Update();int result=nextPoll(event);
 if(result && event && event->type==SDL_WINDOWEVENT && event->window.event==SDL_WINDOWEVENT_FOCUS_LOST){
  Guard guard;
  if(owned && event->window.windowID==windowId(window)){Release();rotating=false;work.store(false,std::memory_order_release);}
 }
 return result;
}
struct Position {float x,y;};
static Position* Virtual(void* self,char* arg,int* value){
 // Validated FF 90 08 01 00 00: the original vtable call at byte offset 0x108.
 auto table=*reinterpret_cast<void***>(self);
 auto result=reinterpret_cast<Position*(*)(void*,char*,int*)>(table[0x108/sizeof(void*)])(self,arg,value);
 if(!virtualWork.load(std::memory_order_acquire))return result;
 Guard guard;
 virtualX=result->x;virtualY=result->y;
 if(virtualState==VirtualState::Override){result->x=static_cast<float>(width/2);result->y=-1000000.0f;}
 else if(virtualState==VirtualState::Restore){result->x=static_cast<float>(savedX);result->y=static_cast<float>(savedY);virtualState=VirtualState::Normal;virtualWork.store(false,std::memory_order_release);}
 return result;
}
static void Shutdown(){
 if(!available)return;
 Guard guard;policy=false;rotating=false;cancel=true;work.store(true,std::memory_order_release);
}
}

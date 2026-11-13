// Uses SDL's cursor API on the client Lua Tick callback.
// Lua requests are applied on that thread; no input events or new hooks.
namespace CursorVisibility {
using ShowFn=int(*)(int);
using RedrawFn=void(*)(void*);
static ShowFn show{};
static RedrawFn redraw{};
static std::atomic<bool> requested{false};
static bool owned=false;
static void Initialize(){
 auto module=GetModuleHandleW(L"SDL2.dll");
 show=module?reinterpret_cast<ShowFn>(GetProcAddress(module,"SDL_ShowCursor")):nullptr;
 redraw=module?reinterpret_cast<RedrawFn>(GetProcAddress(module,"SDL_SetCursor")):nullptr;
}
static int Set(void* L){
 if(At(L,1)->tag!=1)return Se<int(*)(void*,int,char const*)>(RVA_ArgError)(L,1,"boolean expected");
 requested.store(show && redraw && Boolean(L,1),std::memory_order_relaxed);
 PushBool(L,show && redraw);return 1;
}
static void Update(){
 if(!show || !redraw)return;
 if(requested.load(std::memory_order_relaxed)){
  show(0);
  redraw(nullptr);
  owned=true;
 }else if(owned){show(1);redraw(nullptr);owned=false;}
}
static void Shutdown(){requested.store(false,std::memory_order_relaxed);Update();}
}

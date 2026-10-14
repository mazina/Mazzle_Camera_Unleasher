#pragma once
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

// Pure transform, shared by the runtime and focused regression checks.
// Only CharacterMove directions are written. Camera bindings, modifiers,
// explicitly empty arrays and all unrelated preferences are preserved.
namespace InputBindingConfig {
struct Action {char const* camera;char const* character;char const* defaults[3];};
inline constexpr Action Actions[]={
 {"CameraForward","CharacterMoveForward",{"c:leftstick_yneg","key:w","key:up"}},
 {"CameraBackward","CharacterMoveBackward",{"c:leftstick_ypos","key:s","key:down"}},
 {"CameraLeft","CharacterMoveLeft",{"c:leftstick_xneg","key:a","key:left"}},
 {"CameraRight","CharacterMoveRight",{"c:leftstick_xpos","key:d","key:right"}},
 {"CameraRotateLeft",nullptr,{"c:rightstick_xpos","key:q","key:delete"}},
 {"CameraRotateRight",nullptr,{"c:rightstick_xneg","key:e","key:end"}},
 {"CameraToggleMouseRotate",nullptr,{"mouse:middle",nullptr,nullptr}},
};
static bool Build(rapidjson::Document& document,std::string& snapshot,bool& changed,std::string& why){
 using namespace rapidjson;changed=false;
 if(document.HasParseError() || !document.IsObject()){why="input config is not a JSON object";return false;}
 // Validate all owned entries before mutating even the in-memory document.
 for(auto const& action:Actions){
  auto found=document.FindMember(action.camera);
  if(found!=document.MemberEnd()){
   if(!found->value.IsArray()){why=std::string(action.camera)+" is not a binding array";return false;}
   for(auto const& value:found->value.GetArray())if(!value.IsString()){why=std::string(action.camera)+" contains a non-string binding";return false;}
  }
 }
 Document cache(kObjectType);auto& allocator=document.GetAllocator();
 for(auto const& action:Actions){
  Value bindings(kArrayType);auto found=document.FindMember(action.camera);
  if(found!=document.MemberEnd())bindings.CopyFrom(found->value,allocator);
  else for(auto fallback:action.defaults)if(fallback)bindings.PushBack(Value(fallback,allocator),allocator);
  cache.AddMember(Value(action.camera,cache.GetAllocator()),Value(bindings,cache.GetAllocator()),cache.GetAllocator());
  if(!action.character)continue;
  for(auto& value:bindings.GetArray()){
   if(!strcmp(value.GetString(),"c:leftstick_yneg"))value.SetString("c:leftstick_ypos",allocator);
   else if(!strcmp(value.GetString(),"c:leftstick_ypos"))value.SetString("c:leftstick_yneg",allocator);
  }
  auto existing=document.FindMember(action.character);
  if(existing==document.MemberEnd()){document.AddMember(Value(action.character,allocator),bindings,allocator);changed=true;}
  else if(existing->value!=bindings){existing->value.Swap(bindings);changed=true;}
 }
 StringBuffer text;Writer<StringBuffer> writer(text);cache.Accept(writer);snapshot.assign(text.GetString(),text.GetSize());return true;
}
static std::string Serialize(rapidjson::Document const& document){
 rapidjson::StringBuffer text;rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(text);document.Accept(writer);return {text.GetString(),text.GetSize()};
}
}

#pragma once
#include <algorithm>
#include <cwchar>
#include <string>
#include <utility>
#include <vector>
#pragma comment(lib,"version.lib")

namespace RuntimeModule {
struct Candidate {HMODULE module=nullptr;std::wstring path;};
static std::wstring ModulePath(HMODULE module){
 wchar_t path[32768];DWORD count=GetModuleFileNameW(module,path,static_cast<DWORD>(std::size(path)));
 return count && count<std::size(path)?std::wstring(path,count):std::wstring{};
}
static bool IsExtenderFile(std::wstring const& path){
 DWORD ignored{};DWORD bytes=GetFileVersionInfoSizeW(path.c_str(),&ignored);if(!bytes)return false;
 std::vector<unsigned char> version(bytes);if(!GetFileVersionInfoW(path.c_str(),0,bytes,version.data()))return false;
 struct Translation {WORD language,codepage;};Translation* translations=nullptr;UINT length{};
 if(!VerQueryValueW(version.data(),L"\\VarFileInfo\\Translation",reinterpret_cast<void**>(&translations),&length) || length%sizeof(Translation))return false;
 auto matches=[&](Translation entry,wchar_t const* field,wchar_t const* expected){
  wchar_t query[96];swprintf_s(query,L"\\StringFileInfo\\%04x%04x\\%ls",entry.language,entry.codepage,field);
  wchar_t* value=nullptr;UINT characters{};size_t expectedLength=wcslen(expected);
  return VerQueryValueW(version.data(),query,reinterpret_cast<void**>(&value),&characters)
   && characters==expectedLength+1 && wmemcmp(value,expected,expectedLength+1)==0;
 };
 for(UINT i=0;i<length/sizeof(Translation);++i)if(matches(translations[i],L"ProductName",L"BG3 Script Extender") && matches(translations[i],L"FileDescription",L"BG3 Script Extender DLL"))return true;
 return false;
}
// Metadata identifies a candidate. ResolveRuntime still validates its symbols,
// native instruction forms and layouts before any address is used or hook added.
class Finder {
 std::vector<std::pair<HMODULE,std::wstring>> rejected;
public:
 Candidate Find(bool& ambiguous){
  ambiguous=false;
  if(auto named=GetModuleHandleW(L"BG3ScriptExtender.dll"))return {named,ModulePath(named)};
  HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,GetCurrentProcessId());if(snapshot==INVALID_HANDLE_VALUE)return {};
  Candidate found;MODULEENTRY32W entry{};entry.dwSize=sizeof(entry);
  if(Module32FirstW(snapshot,&entry))do{
   if(_wcsicmp(entry.szModule,L"DWrite.dll"))continue;
   auto path=ModulePath(entry.hModule);
   if(path.empty())continue;
   auto identity=std::make_pair(entry.hModule,path);
   if(std::find(rejected.begin(),rejected.end(),identity)!=rejected.end())continue;
   if(!IsExtenderFile(path)){rejected.push_back(std::move(identity));continue;}
   if(found.module){ambiguous=true;break;}
   found={entry.hModule,std::move(path)};
  }while(Module32NextW(snapshot,&entry));
  CloseHandle(snapshot);return ambiguous?Candidate{}:found;
 }
};
}

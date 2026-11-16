@echo off
setlocal
set "BRIDGE_PROJECT_DIR=%~dp0"
if not defined BRIDGE_TOOLS_DIR set "BRIDGE_TOOLS_DIR=%USERPROFILE%\Desktop\BG3_Modding\_MyProjects\_Codex_Tools_Research\Mazzle_Native_Camera_Bridge"
if not defined BRIDGE_VCVARS set "BRIDGE_VCVARS=D:\BG3_Modding\Tools\VisualStudio\BuildTools2026\VC\Auxiliary\Build\vcvars64.bat"
if not defined BRIDGE_DETOURS_ROOT set "BRIDGE_DETOURS_ROOT=D:\BG3_Modding\SEDevelopment\dependencies\56654df550b6e855a04326b6c8ec7602ef23f46a1e11918d656ec33bdccf7b89\External\Detours"
if not defined BRIDGE_JSON_ROOT set "BRIDGE_JSON_ROOT=%BRIDGE_DETOURS_ROOT%\..\rapidjson-master\include"
if not defined BRIDGE_SDL_ROOT set "BRIDGE_SDL_ROOT=D:\BG3_Modding\SEDevelopment\dependencies\current-108da3f7\SDL2-2.30.6\include"
call "%BRIDGE_VCVARS%" -vcvars_ver=14.50.35717
if errorlevel 1 exit /b %errorlevel%
set "BRIDGE_BUILD_FOLDER=build"
set "BRIDGE_DIAGNOSTIC_DEFINE=/DMAZZLE_BRIDGE_DIAGNOSTICS=0"
if "%BRIDGE_DIAGNOSTICS%"=="1" (
    set "BRIDGE_BUILD_FOLDER=build-diagnostics"
    set "BRIDGE_DIAGNOSTIC_DEFINE=/DMAZZLE_BRIDGE_DIAGNOSTICS=1"
)
if not exist "%BRIDGE_TOOLS_DIR%\%BRIDGE_BUILD_FOLDER%" mkdir "%BRIDGE_TOOLS_DIR%\%BRIDGE_BUILD_FOLDER%"
if errorlevel 1 exit /b %errorlevel%
pushd "%BRIDGE_TOOLS_DIR%\%BRIDGE_BUILD_FOLDER%"
if errorlevel 1 exit /b %errorlevel%
ml64 /nologo /c /Zi "%BRIDGE_PROJECT_DIR%keyboard.asm"
if errorlevel 1 goto failed
rc /nologo /i "%BRIDGE_PROJECT_DIR%." /fo bootstrap.res "%BRIDGE_PROJECT_DIR%bootstrap.rc"
if errorlevel 1 goto failed
cl /nologo %BRIDGE_DIAGNOSTIC_DEFINE% /std:c++20 /W4 /WX /O2 /Zi /MD /EHsc /LD /I"%BRIDGE_DETOURS_ROOT%\include" /external:I"%BRIDGE_JSON_ROOT%" /external:I"%BRIDGE_SDL_ROOT%" /external:W0 "%BRIDGE_PROJECT_DIR%bridge.cpp" keyboard.obj bootstrap.res /link /OUT:Mazzle_Camera_Unleasher.dll /PDB:Mazzle_Camera_Unleasher.pdb "%BRIDGE_DETOURS_ROOT%\lib.X64\detours.lib"
if errorlevel 1 goto failed
popd
exit /b 0
:failed
set "BRIDGE_BUILD_RESULT=%errorlevel%"
popd
exit /b %BRIDGE_BUILD_RESULT%

@echo off
setlocal
set "BRIDGE_PROJECT_DIR=%~dp0"
if not defined BRIDGE_TOOLS_DIR set "BRIDGE_TOOLS_DIR=C:\Users\mazzin\Desktop\BG3_Modding\_MyProjects\_Codex_Tools_Research\Mazzle_Native_Camera_Bridge"
if not defined BRIDGE_VCVARS set "BRIDGE_VCVARS=D:\BG3_Modding\Tools\VisualStudio\BuildTools2026\VC\Auxiliary\Build\vcvars64.bat"
if not defined BRIDGE_DETOURS_ROOT set "BRIDGE_DETOURS_ROOT=D:\BG3_Modding\SEDevelopment\dependencies\56654df550b6e855a04326b6c8ec7602ef23f46a1e11918d656ec33bdccf7b89\External\Detours"
call "%BRIDGE_VCVARS%" -vcvars_ver=14.50.35717
if errorlevel 1 exit /b %errorlevel%
if not exist "%BRIDGE_TOOLS_DIR%\build" mkdir "%BRIDGE_TOOLS_DIR%\build"
if errorlevel 1 exit /b %errorlevel%
pushd "%BRIDGE_TOOLS_DIR%\build"
if errorlevel 1 exit /b %errorlevel%
ml64 /nologo /c /Zi "%BRIDGE_PROJECT_DIR%keyboard.asm"
if errorlevel 1 goto failed
cl /nologo /std:c++20 /W4 /WX /O2 /Zi /MD /EHsc /LD /I"%BRIDGE_DETOURS_ROOT%\include" "%BRIDGE_PROJECT_DIR%bridge.cpp" keyboard.obj /link /OUT:MazzleNativeCameraBridgeFull.dll /PDB:MazzleNativeCameraBridgeFull.pdb "%BRIDGE_DETOURS_ROOT%\lib.X64\detours.lib"
if errorlevel 1 goto failed
copy /b /y "%BRIDGE_PROJECT_DIR%bridge.lua" "bridge.lua" >nul
if errorlevel 1 goto failed
popd
exit /b 0
:failed
set "BRIDGE_BUILD_RESULT=%errorlevel%"
popd
exit /b %BRIDGE_BUILD_RESULT%

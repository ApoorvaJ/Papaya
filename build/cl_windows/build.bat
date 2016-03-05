if "%1"=="run" echo run
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64

set CommonCompilerFlags=-Od -EHsc -MTd -nologo -GR- -Zo -W3 -WX -wd4100 -wd4505 -FC -Z7
rem set CommonCompilerFlags=-DHANDMADE_STREAMING=1 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 %CommonCompilerFlags% 
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib kernel32.lib opengl32.lib comdlg32.lib

pushd ..\build\cl_windows

cl %CommonCompilerFlags% ..\..\src\win32_papaya.cpp /link %CommonLinkerFlags% && win32_papaya.exe
del *.obj *.pdb

popd

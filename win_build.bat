@call "C:\Program Files\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86 &
rmdir /S /Q win_build
mkdir win_build
cd win_build
@echo ----------------------------------------- Creating.project
cmake -v -G  "Visual Studio 10"  .. 
@echo -----------------------------------------
pause
@echo ----------------------------------------- Building.Solution
msbuild barzer.sln /p:Configuration=Debug /p:Platform="Win32"
@echo -----------------------------------------
msbuild barzer.sln /p:Configuration=Release /p:Platform="Win32"
@echo -----------------------------------------
pause
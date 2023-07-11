@echo off
mkdir .\bin
copy obj\gpumon\Win32\Debug\gpumon32.dll bin
copy obj\gpumon\x64\Debug\gpumon64.dll bin
copy obj\gpucmd\Win32\Debug\gpucmd.exe bin
copy obj\gpucmd\x64\Debug\gpucmd.exe bin
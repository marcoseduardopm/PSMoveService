@echo off
setlocal

::Select the path to the root opencv folder
set "psCommand="(new-object -COM 'Shell.Application')^
.BrowseForFolder(0,'Please select the root folder for Opencv (ex: c:\OpenCV-3.0.0).',0,0).self.path""
for /f "usebackq delims=" %%I in (`powershell %psCommand%`) do set "OPENCV_ROOT_PATH=%%I"
if NOT DEFINED OPENCV_ROOT_PATH (goto failure)

::Select the path to the root Boost folder
set "psCommand="(new-object -COM 'Shell.Application')^
.BrowseForFolder(0,'Please select the root folder for Boost (ex: c:\boost_1_59_0).',0,0).self.path""
for /f "usebackq delims=" %%I in (`powershell %psCommand%`) do set "BOOST_ROOT_PATH=%%I"
if NOT DEFINED BOOST_ROOT_PATH (goto failure)

:: Write out the paths to a config batch file
del SetBuildVars.bat
echo @echo off >> SetBuildVars.bat
echo set OPENCV_BUILD_PATH=%OPENCV_ROOT_PATH%\build >> SetBuildVars.bat
echo set BOOST_ROOT_PATH=%BOOST_ROOT_PATH% >> SetBuildVars.bat
echo set BOOST_LIB_PATH=%BOOST_ROOT_PATH%\lib32-msvc-12.0 >> SetBuildVars.bat

:: Add MSVC build tools to the path
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86

:: Compile the DEBUG|Win32 and RELEASE|Win32 builds of protobuf
echo "Creating protobuf project files..."
pushd thirdparty\protobuf
del /f /s /q vsprojects > nul
rmdir /s /q vsprojects
mkdir vsprojects
pushd vsprojects
cmake -G "Visual Studio 12 2013" -Dprotobuf_DEBUG_POSTFIX="" -Dprotobuf_BUILD_TESTS=OFF ../cmake
echo "Building protobuf DEBUG|Win32"
MSBuild.exe protobuf.sln /p:configuration=DEBUG /t:Clean;Build 
echo "Building protobuf RELEASE|Win32"
MSBuild.exe protobuf.sln /p:configuration=RELEASE /t:Clean;Build
popd
popd

:: Compile the DEBUG|Win32 and RELEASE|Win32 builds of libusb
pushd thirdparty\libusb\msvc\
echo "Building libusb DEBUG|Win32..."
MSBuild.exe libusb_2013.sln /p:configuration=DEBUG /p:Platform="Win32" /t:Clean;Build 
echo "Building libusb RELEASE|Win32..."
MSBuild.exe libusb_2013.sln /p:configuration=RELEASE /p:Platform="Win32" /t:Clean;Build
popd

:: Compile the RELEASE|Win32 build of SDL2
echo "Creating SDL2 project files..."
pushd thirdparty\SDL2
del /f /s /q build > nul
rmdir /s /q build
mkdir build
pushd build
cmake .. -G "Visual Studio 12 2013" -DDIRECTX=OFF -DDIRECTX=OFF -DSDL_STATIC=ON -DFORCE_STATIC_VCRT=ON -DEXTRA_CFLAGS="-MT -Z7 -DSDL_MAIN_HANDLED -DWIN32 -DNDEBUG -D_CRT_SECURE_NO_WARNINGS -DHAVE_LIBC -D_USE_MATH_DEFINES
echo "Building SDL2 Release|Win32..."
MSBuild.exe SDL2.sln /p:configuration=RELEASE /p:Platform="Win32" /t:Clean
MSBuild.exe SDL2-static.vcxproj /p:configuration=RELEASE /p:Platform="Win32" /t:Build 
MSBuild.exe SDL2main.vcxproj /p:configuration=RELEASE /p:Platform="Win32" /t:Build
popd
popd

:: Generate the project files for PSMoveService
call GenerateProjectFiles_Win32.bat
pause
goto exit

:failure
goto exit

:exit
endlocal

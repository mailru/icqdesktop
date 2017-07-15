if not exist "..\build" mkdir ..\build
cd ..\build
call "%VS110COMNTOOLS%\VsDevCmd.bat"
cmake .. -DAPP_TYPE=%1 -D%2=ON -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 11 2012" -T v110_xp
MSBuild ../build/icq.sln /m /t:Rebuild
exit %errorlevel%

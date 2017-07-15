if not exist "..\build" mkdir ..\build
cd ..\build
cmake .. -DAPP_TYPE=Agent -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 11 2012" -T v110_xp
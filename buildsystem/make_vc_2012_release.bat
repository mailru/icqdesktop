if not exist "..\build" mkdir ..\build
cd ..\build
cmake .. -DAPP_TYPE=ICQ -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 11 2012"
@echo off
:: build.bat — Windows build script (no make required)
:: Requires: gcc in PATH (MinGW / MSYS2 / WinLibs)

echo.
echo [build] Compilando dns_server.exe ...
gcc -Wall -Wextra -std=c11 -g -o dns_server.exe dns_server.c -lws2_32
if errorlevel 1 goto err

echo [build] Compilando web_server.exe ...
gcc -Wall -Wextra -std=c11 -g -o web_server.exe web_server.c -lws2_32
if errorlevel 1 goto err

echo [build] Compilando client.exe ...
gcc -Wall -Wextra -std=c11 -g -o client.exe client.c -lws2_32
if errorlevel 1 goto err

echo.
echo [build] OK — dns_server.exe  web_server.exe  client.exe
echo.
echo Como usar:
echo   1. Abra 3 janelas de terminal nesta pasta.
echo   2. Terminal 1: dns_server.exe
echo   3. Terminal 2: web_server.exe
echo   4. Terminal 3: client.exe
echo.
goto :eof

:err
echo.
echo [build] ERRO — verifique se o gcc esta no PATH.
exit /b 1

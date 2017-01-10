REM create a server/client (27015)
REM --address %addr%

set port=2302
x64\release\sockets.exe --server --port %port%
pause
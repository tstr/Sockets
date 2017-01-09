REM create a server/client (27015)
REM --address %addr%

set port=2302
set addr=86.147.115.187
REM set addr=192.168.1.64
sockets.exe --client --port %port% --address %addr%
pause
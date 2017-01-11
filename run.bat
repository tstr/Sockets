REM create a server/client (27015)
REM --address %addr%

set port=666
REM set addr=localhost
set addr=127.0.0.1
REM set addr=137.44.219.127

start cmd /c "title server && x64\release\sockets.exe --server --port %port% & pause"
timeout /t 3
start cmd /c "title client0 && x64\release\sockets.exe --client --address %addr% --port %port% & pause"
start cmd /c "title client1 && x64\release\sockets.exe --client --address %addr% --port %port% & pause"
start cmd /c "title client2 && x64\release\sockets.exe --client --address %addr% --port %port% & pause"
start cmd /c "title client3 && x64\release\sockets.exe --client --address %addr% --port %port% & pause"


REM http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#closedown

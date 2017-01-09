REM create a server/client (27015)
REM --address %addr%

set port=666
REM set addr=localhost
REM set addr=127.0.0.1
set addr=137.44.219.127

start cmd /c "title server && release\sockets.exe --server --port %port% & pause"
timeout /t 3
start cmd /c "title client && release\sockets.exe --client --address %addr% --port %port% & pause"

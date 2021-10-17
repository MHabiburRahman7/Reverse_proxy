# Reverse_proxy

## Requirement
* Build IDE = Visual Studio 2019 (v142)
* HTTP Testing Tool(?) = Postman or Internet Browser

# Testing
## Build the .exe file
* Open the `.sln` file
* Select the desired architecture at Visual Studio's Toolbar (x86 or x64)
* Make sure the commented section in `Main.cpp` is correct (make sure it is release)
* Build it

## Running
* Locate and run the exe file using `cmd` console by using the following format:

```
C:\Users\User> cd <project location>

C:\Users\User\<project location>> cd <exe directory location>

C:\Users\User\<project location>\<exe directory location>> tcpproxy_server.exe

C:\Users\User\<project location>\<exe directory location>> tcpproxy_server.exe <this_server_ip> <this_server_port> <forward_server_ip> <forward_server_port>

```

* `this_server_ip`: you can define this_server_ip with the unused IPv4 inside your network (127.0.0.1 ~ 127.0.255.255)
* `forward_server_ip`: you can define it with the desired forward IP (eg: 54.156.165.4)
* `this_server_port` & `forward_server_port`: for the sake of simplicity, you can use port 80

* The output file name is `dumpData.txt` and located at the same folder as the `.exe` file

## Testing
* Using your HTTP Testing Tool, try to call `this_server_ip:this_server_port`
* The cmd console will show the transmitted raw data
* The output file will record all the record continuously

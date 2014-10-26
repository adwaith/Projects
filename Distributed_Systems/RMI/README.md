RMI
===

Lab2 of 15-440/640

Remote Method Invocation.

Compile and Build
-----------
`cd` into the folder `src` and build it using
```
make clean
make
```

Execution
-----------
To run the server, execute:
```
./run_server.sh PortNumber
```

To run the client, execute:
```
./run_client.sh HostNameOfServer PortNumberOfServer
```

Examples
-----------
There are 2 examples, one is to perform simple arithmetic operations such as `add`, `sub`, `mul`, and `div`. The other is to convert the case of a String into either `upper` or `lower`. These simple example will show how you can adapt this RMI framework to your needs. More information can be got by reading the report.

# RapidRPC 
a simple and fast RPC framework for C++.

## dependencies
- tinyxml
- protobuf 3.19.4

## build
```shell
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/usr/local
sudo cmake --build build --target install -- -j 10
```

## usage
see [test/test_rpc_server.cc](test/test_rpc_server.cc) and [test/test_rpc_client.cc](test/test_rpc_client.cc)
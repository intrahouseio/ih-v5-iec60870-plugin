cd lib60870-C/examples/cs104_server
sudo gcc -g -o ../../../bin/server104_darwin_arm64 simple_server.c -I../../src/inc/api -I../../src/hal/inc -I../../src/tls ../../build_mac/lib60870_darwin_arm64.a -lpthread

cd lib60870-C/examples/cs104_client
make PROJECT_BINARY_NAME=client104_linux_amd64 TARGET=LINUX-X64
make PROJECT_BINARY_NAME=client104_linux_arm TARGET=LINUX-ARM
make PROJECT_BINARY_NAME=client104_linux_arm64 TARGET=LINUX-ARM64
make PROJECT_BINARY_NAME=client104_windows_amd64.exe TARGET=WIN64

#sudo gcc -g -o client104_linux_amd64 simple_client_2.c -I../../src/inc/api -I../../src/hal/inc -I../../src/tls ../../build/liblib60870.a -lpthread
#sudo arm-linux-gnueabi-gcc -mno-unaligned-access -g -o client104_linux_arm simple_client_2.c -I../../src/inc/api -I../../src/hal/inc -I../../src/tls ../../build-arm/liblib60870.a -lpthread
#sudo aarch64-linux-gnu-gcc -g -o client104_linux_arm64 simple_client_2.c -I../../src/inc/api -I../../src/hal/inc -I../../src/tls ../../build-arm64/liblib60870.a -lpthread
#sudo x86_64-w64-mingw32-gcc -g -DWIN32 -m64 -DEXCLUDE_ETHERNET_WINDOWS  -g -o client104_windows_amd64 simple_client_2.c -I../../src/inc/api -I../../src/hal/inc -I../../src/tls ../../build_win32/liblib60870.a -lws2_32
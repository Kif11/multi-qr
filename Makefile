CXX = g++
CXXFLAGS = -g -O1 -std=c++11 `pkg-config --cflags --libs /usr/local/Cellar/opencv/3.3.1_1/lib/pkgconfig/opencv.pc` -arch x86_64 /usr/local/lib/libzbar.dylib

main:
	$(CXX) $(CXXFLAGS) -o bin/multiqr src/multiqr.cpp include/Base64.cpp include/qrgen/BitBuffer.cpp include/qrgen/QrCode.cpp include/qrgen/QrSegment.cpp

tile:
	$(CXX) $(CXXFLAGS) -o bin/tile src/tile.cpp

install:
	install ./bin/multiqr /usr/local/bin
	install ./bin/tile /usr/local/bin
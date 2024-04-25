CXX=g++
CXXFLAGS=-Wall -Wextra

# Use $USER environment variable to dynamically set the path
OPENSSL_DIR=/sgoinfre/goinfre/Perso/$(USER)/homebrew/Cellar/openssl@3/3.3.0
OPENSSL_ETC_DIR=/sgoinfre/goinfre/Perso/$(USER)/homebrew/etc/openssl@3

LIBS=-L$(OPENSSL_DIR)/lib -lssl -lcrypto
INCLUDES=-I$(OPENSSL_DIR)/include

all: server client

server: server.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -std=c++98 -o server server.cpp $(LIBS)

client: client.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -std=c++14 -o client client.cpp $(LIBS)

clean:
	rm -f server client

appname = webclient

CXXFLAGS = -O2 -g -Wall -Wextra -pedantic -std=c++11 -g
LDFLAGS = -Wl,-rpath=/usr/local/lib/gcc49/
CXX = g++

all: $(appname)

default: $(appname)

$(appname): $(appname).cpp

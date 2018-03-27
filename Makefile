# CXXFLAGS=-DReadFromFile -g # for debugging
CXXFLAGS=-DReadFromFile

ifeq ($(OS),Windows_NT)
  CC=gcc
  LDFLAGS=-lws2_32
endif

all: RFInspect

RFInspect: RCSwitch.o RFInspect.o tinywav.o
	$(CXX) $(CXXFLAGS) $+ $(LDFLAGS) -o $@

clean:
	$(RM) *.o

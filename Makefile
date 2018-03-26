# CXXFLAGS=-DReadFromFile -g # for debugging
CXXFLAGS=-DReadFromFile

all: RFInspect

RFInspect: RCSwitch.o RFInspect.o tinywav.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $+ -o $@

clean:
	$(RM) *.o

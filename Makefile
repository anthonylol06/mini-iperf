CXX = gcc

all: miniIPerf

SOURCES = miniIPerf.c
miniIPerf: $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o miniIPerf

clean:
	rm -rf miniIPerf *.dSYM

.PHONY: clean
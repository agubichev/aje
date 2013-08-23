CXX=g++
CXXFLAGS = -g -Wall -Wextra  -Wno-type-limits --std=c++11
DEPTRACKING=-MD -MF $(@:.o=.d)
BUILDEXE=$(CXX) -o$@ $(CXXFLAGS) $(LDFLAGS) $^
CHECKDIR=@mkdir -p $(dir $@)

all: bin/topics$(EXEEXT)


include src/LocalMakefile

-include bin/*.d bin/*/*.d


bin/%.o: src/%.cpp
	$(CHECKDIR)
	$(CXX) -o$@ -c $(CXXFLAGS) $(DEPTRACKING) $<


clean:
	find bin -name '*.d' -delete -o -name '*.o' -delete -o '(' -perm -u=x '!' -type d '!' -name '*.sh' ')' -delete


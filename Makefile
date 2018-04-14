CXXFLAGS=-std=c++11 -Wall -Wextra -Werror $(EXTRA_CXXFLAGS)
LDFLAGS=$(EXTRA_LDFLAGS)

uncasc: uncasc.cpp
		$(CXX) -o uncasc $(CXXFLAGS) $(LDFLAGS) uncasc.cpp -lcasc

format:
		clang-format -i uncasc.cpp

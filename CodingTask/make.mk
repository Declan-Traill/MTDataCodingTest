CXX = g++
CXXFLAGS = -std=c++11 -Wall -I/usr/include/jsoncpp -I/usr/include
LDFLAGS = -lboost_system -ljsoncpp

TARGET = scales_parser
SOURCES = main.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

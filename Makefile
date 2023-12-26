
CXX = g++
LD = g++
CXXFLAGS = -std=c++17 -pedantic-errors -Wall -Werror -O2 -fsanitize=address
LDFLAGS = -fsanitize=address

BUILDPATH = build
SOURCES = main.cpp
HEADERS =
TARGET = lab_vm

OBJECTS = $(SOURCES:%.cpp=$(BUILDPATH)/%.o)

.PHONY: all build clean

all: build

build: $(TARGET)

clean:
	@rm -vrf $(BUILDPATH) 2> /dev/null
	@rm -vf $(TARGET) 2> /dev/null

%.cpp:

$(OBJECTS): $(BUILDPATH)/%.o : %.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(TARGET): $(OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS)

CXX ?= c++
CPPFLAGS ?= -Iinclude -DHUM_VERSION=\"0.3.0\"
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Wpedantic
LDFLAGS ?=

SOURCES := src/main.cpp src/style.cpp src/confirm.cpp src/join.cpp src/log.cpp \
	src/input.cpp src/choose.cpp src/spin.cpp src/terminal.cpp src/text.cpp

SOURCES += src/pager.cpp src/filter.cpp src/file.cpp src/table.cpp src/write.cpp \
	src/viewport.cpp
OBJECTS := $(SOURCES:.cpp=.o)
TARGET := hum
TEST_TARGET := tests/terminal_test

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(TEST_TARGET): tests/terminal_test.cpp src/terminal.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

test: $(TARGET) $(TEST_TARGET)
	sh ./tests/test_cli.sh ./$(TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS) $(TEST_TARGET)

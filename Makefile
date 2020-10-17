CC = g++

CFLAGS = -c -O3 -std=c++17 -Wall  -I.
LDFLAGS = -ldl -lstdc++ -lstdc++fs

SOURCES = aiml.cpp

OBJECTS = $(SOURCES:.cpp=.o)
OBJECTS := $(addprefix obj/,$(OBJECTS))

EXECUTABLE = alice

all: obj $(SOURCES) $(EXECUTABLE)

obj:
	@mkdir -p obj

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

obj/%.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -rf obj/*.o $(EXECUTABLE)

SRC := src
LIB := lib
OBJ := obj
BIN := bin
EXTDIR := external
TARGET := application

LIBS := -lpthread -lrt -l sqlite3
CFLAGS := -Wall -O3 -pedantic

SOURCES := $(wildcard $(SRC)/*.c) $(wildcard $(SRC)/$(LIB)/*.c)
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

.PHONY: setup dir external run doc

all: setup external $(TARGET)
setup: dir
build: $(TARGET)
rebuild: clean all

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $(BIN)/$@  $(LIBS)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -I$(SRC) -c $(CFLAGS) $< -o $@

external:
	cd $(EXTDIR) && $(MAKE) all

doc:
	doxygen doxygen.config

run: setup external $(TARGET)
	@$(BIN)/$(TARGET)

dir:
	@mkdir -p $(OBJ)
	@mkdir -p $(OBJ)/$(LIB)
	@mkdir -p $(BIN)

clean:
	cd $(EXTDIR) && $(MAKE) clean
	rm -rf $(OBJ)
	rm -rf $(BIN)
	rm -f vgcore.*

TARGET = c-ray
CC = gcc

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	FRAMEWORKS = -lSDL2
else
	FRAMEWORKS = -I/usr/local/include -L/usr/local/lib -lSDL2
endif

ifeq ($(usesdl),no)
	FRAMEWORKS = -I/usr/local/include
endif

CFLAGS = -std=c99 -Wall
LINKER = gcc -o
LFLAGS = -I. -lm -pthread $(FRAMEWORKS)

FRAMEWORK_PATH = /Library/Frameworks

SRCDIR = src
OBJDIR = obj
BINDIR = bin

SOURCES := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
rm = rm -f

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(LINKER) $@ $(LFLAGS) $(OBJECTS) $(FRAMEWORKS) -lm
	@echo "Linking complete..."

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@ $(FRAMEWORKS)
	@echo "Compiled "$<" successfully"

.PHONY: clean
clean:
	@$(rm) $(OBJECTS)
	@echo "Cleanup done"

.PHONY: remove
remove: clean
	@$(rm) $(BINDIR)/$(TARGET)
	@echo "Binary removed"

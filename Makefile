TARGET = c-ray
CC = gcc
CFLAGS = -std=c99 -Wall
LINKER = gcc -o
LFLAGS = -I. -lm -pthread

SRCDIR = src
OBJDIR = obj
BINDIR = bin

SOURCES := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
rm = rm -f

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(LINKER) $@ $(LFLAGS) $(OBJECTS) -lm
	@echo "Linking complete..."

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully"

.PHONY: clean
clean:
	@$(rm) $(OBJECTS)
	@echo "Cleanup done"

.PHONY: remove
remove: clean
	@$(rm) $(BINDIR)/$(TARGET)
	@echo "Binary removed"

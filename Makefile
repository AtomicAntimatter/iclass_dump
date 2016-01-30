##### MAKEFILE #####
# Generic Layout
# by Sharan ***REMOVED***
# 1/30/2016

TARGET				:= iClass_Dump

LIBS					:= -lftd2xx

SRCDIR				:= src
INCDIR				:= include
LIBDIR				:= lib
BUILDDIR			:= build
BINDIR				:= bin

# PKGS					:=

# Should not need to modify anything below this for most applications

# PKGS_CFLAGS		:= `pkg-config --cflags $(PKGS)`
# PKGS_LDFLAGS	:= `pkg-config --libs $(PKGS)`

CC						:= gcc

CFLAGS				:= -Wall -g
# CFLAGS				 += $(PKGS_CFLAGS)
CFLAGS				+= -I$(INCDIR)

LDFLAGS			 	:= $(PKGS_LDFLAGS)
LDFLAGS				+= -L./$(LIBDIR) -Wl,-R./$(LIBDIR) '-Wl,-R$$ORIGIN'
LDFLAGS				+= $(LIBS)

SOURCES				:= $(wildcard $(SRCDIR)/*.c)
OBJECTS				:= $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

rm						:= rm -f

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(CC) $^ -o $@ $(LDFLAGS)
	@echo "Linking complete!"

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)
	@echo "Compiled "$<" successfully!"

.PHONEY: clean
clean:
	@$(rm) $(OBJECTS)
	@echo "Cleanup complete!"

.PHONEY: remove
remove: clean
	@$(rm) $(BINDIR)/$(TARGET)
	@echo "Executable removed!"

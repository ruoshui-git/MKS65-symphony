# Thie is working now! Most of credits to: http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/
# Also referenced:
# make manual: https://www.gnu.org/software/make/manual/html_node/index.html
# Ruoshui on 20191205

# Email complaints to: rmao10@stuy.edu


# space-separated list of source files
SRCS = shell.c server_thread.c utils.c cmd_handlers.c socket.c \
	midi_player.c midi_reader.c midi_writer.c midi_splitter.c midi.c midifile.c

# space-separated list of libraries, if any,
# each of which should be prefixed with -l
LIBS = -pthread -lfluidsynth -lreadline 

# name for executable
EXE = main.out

# USAGE: Change above variables to suit your needs

ifeq ($(filter $(DEBUG), false f FALSE F), )
	DEBUG_FLAG = -ggdb3
else
    DEBUG_FLAG = -DNDEBUG
endif

CSTD = gnu11 # not c11

# flags to pass compiler
CFLAGS = $(DEBUG_FLAG) -std=$(CSTD) -I ~/mysys/include -L ~/mysys/lib64 -L ~/mysys/lib -DSERVER_NOPLAY

# compiler to use
CC = gcc

# Ruoshui: my computer doesn't have gcc; so this will change CC to clang if "which gcc" outputs nothing
ifeq (, $(shell which gcc))
	CC = clang
endif

OBJDIR := obj

# automatically generated list of object files
OBJS = $(SRCS:%.c=$(OBJDIR)/%.o)

# default target
$(EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)



DEPDIR := $(OBJDIR)/.deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

$(OBJDIR)/%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(COMPILE.c) $(OUTPUT_OPTION)  $<

$(DEPDIR): ; @mkdir -p $@

DEPFILES := $(SRCS:%.c=$(DEPDIR)/%.d)
$(DEPFILES):

include $(wildcard $(DEPFILES))


.PHONY: clean run autorun memcheck

# housekeeping
clean:
	rm -f $(EXE) *.out
	rm -rf obj

run:
	./$(EXE)

autorun: $(EXE)
	./$(EXE)

memcheck:
	valgrind --leak-check=summary ./$(EXE)
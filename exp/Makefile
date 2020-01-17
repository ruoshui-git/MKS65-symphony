

LIBS = -lfluidsynth -lpulse -lpulse-simple
EXE = driver
TARGET = pulsedriver.c
DFLAGS = -ggdb3

test:
	gcc $(TARGET) -o $(EXE) $(DFLAGS) $(LIBS)

clean:
	rm -f *.o *.out $(EXE)

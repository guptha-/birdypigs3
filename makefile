DFLAG = -g
WFLAG = -Wall
C11FLAG = -std=c++0x
THREADFLAG = -pthread

SRCDIR = src
OBJDIR = obj
INCDIR = inc
MVOBJ = mv -f *.o obj/

SRCD = dbmain.cpp\
			 dbmsg.cpp\
			 PracticalSocket.cpp
INCD = dbinc.h\
       comconst.h\
			 PracticalSocket.h
OBJD = $(SRCD:.cpp=.o)
DSRC = $(patsubst %,$(SRCDIR)/%,$(SRCD))
DOBJ = $(patsubst %,$(OBJDIR)/%,$(OBJD))
DINC = $(patsubst %,$(INCDIR)/%,$(INCD))

SRCC = birdmain.cpp\
			 birdmsg.cpp\
			 PracticalSocket.cpp
INCC = birdinc.h\
       birdprot.h\
       comconst.h\
			 PracticalSocket.h
OBJC = $(SRCC:.cpp=.o)

CSRC = $(patsubst %,$(SRCDIR)/%,$(SRCC))
COBJ = $(patsubst %,$(OBJDIR)/%,$(OBJC))
CINC = $(patsubst %,$(INCDIR)/%,$(INCC))

SRCP = pigmain.cpp\
			 pigmsg.cpp\
			 PracticalSocket.cpp
INCP = piginc.h\
			 pigds.h\
			 comconst.h\
			 PracticalSocket.h
OBJP = $(SRCP:.cpp=.o)

PSRC = $(patsubst %,$(SRCDIR)/%,$(SRCP))
POBJ = $(patsubst %,$(OBJDIR)/%,$(OBJP))
PINC = $(patsubst %,$(INCDIR)/%,$(INCP))

CREATEDIR = mkdir -p obj bin

all: bird pig db

bird: $(COBJ)
	$(CREATEDIR)
	g++ -o bin/bird $(WFLAG) $(COBJ) -lm $(THREADFLAG)

obj/PracticalSocket.o: src/PracticalSocket.cpp $(CINC)
	$(CREATEDIR)
	g++ -c src/PracticalSocket.cpp -I inc $(C11FLAG) $(WFLAG) $(DFLAG) -o obj/PracticalSocket.o

obj/birdmain.o: src/birdmain.cpp $(CINC)
	$(CREATEDIR)
	g++ -c src/birdmain.cpp -I inc $(C11FLAG) $(WFLAG) $(DFLAG) -o obj/birdmain.o

obj/birdmsg.o: src/birdmsg.cpp $(CINC)
	$(CREATEDIR)
	g++ -c src/birdmsg.cpp -I inc $(C11FLAG) $(WFLAG) $(DFLAG) -o obj/birdmsg.o

pig: $(POBJ)
	$(CREATEDIR)
	g++ -o bin/pig $(WFLAG) $(POBJ) -lm $(THREADFLAG)

obj/pigmain.o: src/pigmain.cpp $(PINC)
	$(CREATEDIR)
	g++ -c src/pigmain.cpp -I inc $(C11FLAG) $(WFLAG) $(DFLAG) -o obj/pigmain.o

obj/pigmsg.o: src/pigmsg.cpp $(PINC)
	$(CREATEDIR)
	g++ -c src/pigmsg.cpp -I inc $(C11FLAG) $(WFLAG) $(DFLAG) -o obj/pigmsg.o

db: $(DOBJ)
	$(CREATEDIR)
	g++ -o bin/db $(WFLAG) $(DOBJ) -lm $(THREADFLAG)

obj/dbmain.o: src/dbmain.cpp $(DINC)
	$(CREATEDIR)
	g++ -c src/dbmain.cpp -I inc $(C11FLAG) $(WFLAG) $(DFLAG) -o obj/dbmain.o

obj/dbmsg.o: src/dbmsg.cpp $(DINC)
	$(CREATEDIR)
	g++ -c src/dbmsg.cpp -I inc $(C11FLAG) $(WFLAG) $(DFLAG) -o obj/dbmsg.o

clean:
	rm -rf bin/*
	rm -rf obj/*

DBG = -g
OPT = -O0
CXXFLAGS += $(OPT) $(DGB)
LDFLAGS += -lSDL
#Source Files

AUDIERE_INCLUDE=/home/blucia0a/cvsandbox/Apps/audiere/include
AUDIERE_LIB=/home/blucia0a/cvsandbox/Apps/audiere/lib

CXXFLAGS += -I$(AUDIERE_INCLUDE)
LDFLAGS += -L$(AUDIERE_LIB) -laudiere

SRCS = Chango.cpp
OBJS = $(SRCS:%.cpp=%.o)
CHANGO=Chango

all: $(CHANGO)

## build rules

%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(CHANGO): $(OBJS)
	$(CXX) -g $(LDFLAGS) -o $@ $+

## cleaning
clean:
	-rm -f *.o Chango

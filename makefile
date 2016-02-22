APP=bin/drunner-install
CC=gcc
#CXX=g++
CXX=bin/colorgcc.pl
RM=rm -f
INC=-Isource -Ibuildnum

CPPFLAGS=-g -Wall -std=c++11 $(BUILD_NUMBER_LDFLAGS) $(INC)
LDFLAGS=-lboost_filesystem -lboost_system
LDLIBS=

HDRS=$(shell find source -maxdepth 1 -name "*.h")
SRCS=$(shell find source -maxdepth 1 -name "*.cpp")
OBJS=$(patsubst source/%,objs/%,$(SRCS:.cpp=.o))
#$(subst .cpp,.o,$(SRCS))

all: $(APP)

$(APP): permissions buildnum/build_number.h $(OBJS) makefile
	$(CXX) $(LDFLAGS) -o $(APP) $(OBJS) $(LDLIBS)

objs/%.o: source/%.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<


depend: .depend

.depend: $(SRCS)
	rm -f ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

dist-clean: clean
	$(RM) *~ .depend

buildnum/build_number.h: $(SRCS) $(HDRS) buildnum/major_version
	@echo
	@echo Bumping build number..
	buildnum/make_buildnum.sh

permissions:
	chmod 0644 $(SRCS) $(HDRS) buildnum/*
	chmod 0755 bin/* buildnum buildnum/make_buildnum.sh	

include .depend

upload: $(APP)
	cd bin/ ; upload drunner-install

push: $(APP)
	git pull ; git add . ; git commit ; git push

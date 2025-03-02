FILES = util file_util id3_parse id3_hash hashtable id3_editor test
MAINFILES = util file_util id3_parse id3_hash hashtable id3_editor
TESTFILES = util file_util id3_parse id3_hash hashtable test
DEPDIR := .deps
OUTDIR := out
CC := gcc
SRCS = $(addsuffix .c,$(FILES))
OBJS = $(addprefix $(OUTDIR)/,$(addsuffix .o,$(MAINFILES)))
TESTOBJS = $(addprefix $(OUTDIR)/,$(addsuffix .o,$(TESTFILES)))

id3_editor: $(OUTDIR) $(OBJS) 
	$(CC) -Wall -g $(OBJS) -o $@ 

test: $(OUTDIR) $(TESTOBJS) # Compile and run tests
	$(CC) -Wall -g $(TESTOBJS) -o $@
	# ./$@

install: id3_editor
	cp id3_editor /usr/bin/id3_editor	

clean:
	rm -rf $(DEPDIR) $(OUTDIR)/*.o id3_editor test

include Makefile.d

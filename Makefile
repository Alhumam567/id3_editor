FILES = util file_util id3_parse id3_hash hashtable id3_editor
DEPDIR := .deps
OUTDIR := out
SRCS = $(addsuffix .c,$(FILES))
OBJS = $(addprefix $(OUTDIR)/,$(addsuffix .o,$(FILES)))

id3_editor.exe : $(OUTDIR) $(OBJS) 
	$(CC) $(OBJS) -o $@ 

clean:
	rm -rf $(DEPDIR) $(OUTDIR)/*.o *.exe

include Makefile.d

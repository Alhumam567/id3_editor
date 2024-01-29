ifeq ($(OS),Windows_NT)
	FLAGS := -Wall -g
else
	FLAGS := -fsanitize=address -Wall -g
endif
OUT := out/
HT_FILES := id3_hash.o hashtable.o
TEST_FILES := $(addprefix $(OUT),util.o file_util.o $(HT_FILES) id3_parse.o test.o)
FILES := $(addprefix $(OUT),util.o file_util.o $(HT_FILES) id3_parse.o id3_editor.o)

id3_editor.exe: $(FILES)
	gcc $(FLAGS) $(FILES) -o $@

test.exe: $(TEST_FILES)
	gcc $(FLAGS) $(TEST_FILES) -o $@

search.exe: id3_hash.o search.o
	gcc $(FLAGS) $(OUT)id3_hash.o $(OUT)search.o -o $@

$(OUT)%.o: %.c 
	gcc $(FLAGS) -c $< -o $@

clean:
	rm -f $(OUT)*.o $(OUT)*.exe

cleancomp: clean id3_editor.exe test.exe

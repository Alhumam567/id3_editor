id3_editor.exe: util.o id3_hash.o hashtable.o id3_parse.o file_util.o id3_editor.o 
	gcc -fsanitize=address -Wall -g id3_editor.o id3_parse.o file_util.o util.o id3_hash.o hashtable.o -o id3_editor.exe

test.exe: util.o file_util.o id3_hash.o hashtable.o id3_parse.o test.o
	gcc -fsanitize=address -Wall -g util.o file_util.o id3_parse.o test.o id3_hash.o hashtable.o -o test.exe

search.exe: id3_hash.o search.o
	gcc -fsanitize=address -Wall -g id3_hash.o search.o -o search.exe

%.o: %.c 
	gcc -fsanitize=address -g -Wall -c $< -o $@

clean:
	rm -f *.o *.exe

cleancomp: clean id3_editor.exe test.exe

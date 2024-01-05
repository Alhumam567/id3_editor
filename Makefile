id3_editor.exe: util.o id3_parse.o file_util.o id3_editor.o 
	gcc -Wall -g id3_editor.o id3_parse.o file_util.o util.o -o id3_editor.exe

test.exe: util.o id3_parse.o test.o
	gcc -Wall -g util.o id3_parse.o test.o -o test.exe

search.exe: id3_hash.o search.o
	gcc -Wall -g id3_hash.o search.o -o search.exe

%.o: %.c 
	gcc -g -Wall -c $< -o $@

clean:
	rm -f *.o *.exe

testclean: 
	rm -f util.o id3_parse.o test.o

searchclean:
	rm -f id3_hash.o search.o
	
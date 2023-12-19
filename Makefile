id3_editor.exe: util.o id3_parse.o file_util.o id3_editor.o 
	gcc -fsanitize=address -g id3_editor.o id3_parse.o file_util.o util.o -o id3_editor.exe

test.exe: util.o id3_parse.o test.o
	gcc -fsanitize=address -g util.o id3_parse.o test.o -o test.exe

%.o: %.c 
	gcc -fsanitize=address -g -Wall -c $< -o $@

testclean: 
	rm -f util.o id3_parse.o test.o

clean:
	rm -f id3_editor.o id3_parse.o file_util.o util.o test.o id3_editor.exe test.exe
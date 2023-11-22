id3_editor.exe: util.o id3_parse.o file_util.o id3_editor.o 
	gcc id3_editor.o id3_parse.o file_util.o util.o -o id3_editor.exe

test.exe: util.o id3_parse.o test.o
	gcc util.o id3_parse.o test.o -o test.exe

%.o: %.c
	gcc -Wall -c $< -o $@

clean:
	rm -f id3_editor.o id3_parse.o file_util.o util.o test.o id3_editor.exe test.exe
id3_editor.exe: util.o id3_parse.o file_util.o id3_editor.o 
	gcc id3_editor.o id3_parse.o file_util.o util.o -o id3_editor.exe

%.o: %.c
	gcc -Wall -c $< -o $@

clean:
	rm -f id3_editor.o id3_parse.o file_util.o util.o id3_editor.exe
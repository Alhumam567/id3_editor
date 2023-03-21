Remove-Item -Force t.mp3
Remove-Item -Force misc/t.hexdump
cp testfile.mp3 t.mp3
gcc -o ./mp3.exe .\mp3_metadata.c
./mp3.exe t.mp3
Format-Hex -Path "t.mp3" > "misc/t.hexdump"
gcc -c src/assembly.c -I include/ -o lib/assembly.o
gcc -c src/dsctbl.c -I include/ -o lib/dsctbl.o
gcc -c src/fifo32.c -I include/ -o lib/fifo32.o
gcc -c src/int.c -I include/ -o lib/int.o
gcc -c src/keyboard.c -I include/ -o lib/keyboard.o
gcc -c src/memory.c -I include/ -o lib/memory.o
gcc -c src/mtask.c -I include/ -o lib/mtask.o
gcc -c src/timer.c -I include/ -o lib/timer.o
gcc -c src/file.c -I include/ -o lib/file.o
ar rcs -o lib/libtest.a lib/*.o
gcc -pthread src/main.c -I include/ -L lib/ -l test -o main.out
./main.out
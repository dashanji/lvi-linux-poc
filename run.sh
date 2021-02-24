gcc -c asmhelper.S
gcc -c main.c
gcc -o main main.o asmhelper.o -lpthread 
./main

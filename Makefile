run: repl
	./repl

repl: mpc.c repl.c
	cc -std=c99 -Wall repl.c mpc.c -ledit -lm -o repl

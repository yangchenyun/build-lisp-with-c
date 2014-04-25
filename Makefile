run: repl
	@./repl

repl: mpc.c repl.c
	cc -std=c99 -Wall repl.c mpc.c -ledit -lm -o repl

debug: debug_repl
	@gdb ./debug_repl

debug_repl: mpc.c repl.c
	cc -std=c99 -g -O0 -Wall repl.c mpc.c -ledit -lm -o debug_repl

runex: example
	@./example

example: mpc.c repl.c
	cc -std=c99 -Wall example.c mpc.c -ledit -lm -o example

all:
	g++ src/*.cpp -Iinclude -o riscvemu
clean:
	rm ./riscvemu

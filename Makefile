all:
	cmake -B build -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw64.cmake
	cmake --build ./build/
	cp ./build/gmcl_riscv-em_win64.dll /mnt/hmm/SteamLibrary/steamapps/common/GarrysMod/garrysmod/lua/bin/

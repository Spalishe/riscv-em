
#pragma once
#include "../include/GarrysMod/Lua/Interface.h"
#include "./include/machine.hpp"
#include <cstdint>

struct RVMachine
{
	RVMachine(uint64_t mem_size, uint8_t hart_count) : mem_size(mem_size), hart_count(hart_count), machine(mem_size, hart_count)
	{
	}

	~RVMachine()
	{
		machine.stop();
	};

	uint64_t mem_size;
	uint8_t hart_count;
	Machine machine;
};
static int s_RVMachine_TypeId;

LUA_FUNCTION_DECLARE(Create_RVMachine);
LUA_FUNCTION_DECLARE(Destroy_RVMachine);
LUA_FUNCTION_DECLARE(RVMachine_GetState);
LUA_FUNCTION_DECLARE(RVMachine_InitMmap);

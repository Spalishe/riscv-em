
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
extern int s_RVMachine_TypeId;

LUA_FUNCTION_DECLARE(Create_RVMachine);
LUA_FUNCTION_DECLARE(Destroy_RVMachine);
LUA_FUNCTION_DECLARE(RVMachine_ToString);
LUA_FUNCTION_DECLARE(RVMachine_Index);
LUA_FUNCTION_DECLARE(RVMachine_GetState);
LUA_FUNCTION_DECLARE(RVMachine_InitMmap);
LUA_FUNCTION_DECLARE(RVMachine_PutFirmware);
LUA_FUNCTION_DECLARE(RVMachine_PutKernel);
LUA_FUNCTION_DECLARE(RVMachine_PutImage);
LUA_FUNCTION_DECLARE(RVMachine_PutCustomDTB);
LUA_FUNCTION_DECLARE(RVMachine_SetBootArgs);
LUA_FUNCTION_DECLARE(RVMachine_InitAutoDevices);
LUA_FUNCTION_DECLARE(RVMachine_Run);
LUA_FUNCTION_DECLARE(RVMachine_Stop);
LUA_FUNCTION_DECLARE(RVMachine_Reboot);

#include "rvmachine.hpp"
#include "../include/GarrysMod/Lua/Interface.h"

using namespace GarrysMod::Lua;

LUA_FUNCTION(Create_RVMachine)
{
	uint64_t memsize   = (uint64_t)LUA->CheckNumber(1);
	uint8_t hart_count = (uint8_t)LUA->CheckNumber(2);

	RVMachine* machine = new RVMachine(memsize, hart_count);
	LUA->PushUserType_Value(machine, s_RVMachine_TypeId);
	return 1;
}
LUA_FUNCTION(Destroy_RVMachine)
{
	RVMachine* machine = LUA->GetUserType<RVMachine>(1, s_RVMachine_TypeId);
	delete machine;
	return 0;
}
LUA_FUNCTION(RVMachine_GetState)
{
	RVMachine* machine = LUA->GetUserType<RVMachine>(1, s_RVMachine_TypeId);

	LUA->PushNumber((int)machine->machine.state.load());
	return 1;
}
LUA_FUNCTION(RVMachine_InitMmap)
{
	RVMachine* machine = LUA->GetUserType<RVMachine>(1, s_RVMachine_TypeId);

	machine->machine.init_mmap();
	return 0;
}

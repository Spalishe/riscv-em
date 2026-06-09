#include "rvmachine.hpp"
#include "../include/GarrysMod/Lua/Interface.h"
#include "limits.h"
#include <unistd.h>

/*
*	if(!LUA->IsType(2, GarrysMod::Lua::Type::File))
	{
		LUA->ThrowError("Bad argument #2 (expected file)");
		return 0;
	}
	LUA->GetField(2, "Read");
	LUA->Push(2);
	LUA->PushNil();

	LUA->Call(2, 1);
* Useful for file_class read
*/

using namespace GarrysMod::Lua;
int s_RVMachine_TypeId = 0;

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
	RVMachine** pp = LUA->GetUserType<RVMachine*>(1, s_RVMachine_TypeId);
	if(!pp || !*pp)
	{
		LUA->ThrowError("Invalid RVMachine userdata");
		return 0;
	}
	RVMachine* machine = *pp;

	if(machine->machine.bios_file)
	{
		fclose(machine->machine.bios_file);
	}
	if(machine->machine.kernel_file)
	{
		fclose(machine->machine.kernel_file);
	}
	if(machine->machine.image_file)
	{
		fclose(machine->machine.image_file);
	}
	delete machine;
	return 0;
}

LUA_FUNCTION(RVMachine_ToString)
{
	RVMachine** pp = LUA->GetUserType<RVMachine*>(1, s_RVMachine_TypeId);
	if(!pp || !*pp)
	{
		LUA->ThrowError("Invalid RVMachine userdata");
		return 0;
	}
	RVMachine* machine = *pp;
	char outstr[64];
	snprintf(outstr, sizeof(outstr), "RISC-V machine: [state %d]\0", (int)machine->machine.state.load());

	LUA->PushString(outstr);
	return 1;
}

LUA_FUNCTION(RVMachine_Index)
{
	if(!LUA->IsType(2, Type::String))
	{
		LUA->PushNil();
		return 1;
	}

	LUA->GetMetaTable(1); // metatable

	LUA->Push(2);	 // key
	LUA->RawGet(-2); // metatable[key]

	return 1;
}
LUA_FUNCTION(RVMachine_GetState)
{
	RVMachine** pp = LUA->GetUserType<RVMachine*>(1, s_RVMachine_TypeId);
	if(!pp || !*pp)
	{
		LUA->ThrowError("Invalid RVMachine userdata");
		return 0;
	}
	RVMachine* machine = *pp;

	LUA->PushNumber((int)machine->machine.state.load());
	return 1;
}
LUA_FUNCTION(RVMachine_InitMmap)
{
	RVMachine** pp = LUA->GetUserType<RVMachine*>(1, s_RVMachine_TypeId);
	if(!pp || !*pp)
	{
		LUA->ThrowError("Invalid RVMachine userdata");
		return 0;
	}
	RVMachine* machine = *pp;

	machine->machine.init_mmap();
	return 0;
}

FILE* openFileInData(std::string path)
{
	char currentDir[PATH_MAX];
	getcwd(currentDir, PATH_MAX);

	char fpath[PATH_MAX];
	sprintf(fpath, "%s\\garrysmod\\data\\%s", currentDir, path.c_str());

	FILE* file = fopen(fpath, "rb");
	return file;
}

LUA_FUNCTION(RVMachine_PutFirmware)
{
	RVMachine** pp = LUA->GetUserType<RVMachine*>(1, s_RVMachine_TypeId);
	if(!pp || !*pp)
	{
		LUA->ThrowError("Invalid RVMachine userdata");
		return 0;
	}
	RVMachine* machine = *pp;

	std::string path = LUA->CheckString(2);

	FILE* bios = openFileInData(path);
	if(!bios)
	{
		LUA->ThrowError("Invalid file!");
		return 0;
	}
	if(machine->machine.bios_file)
	{
		fclose(machine->machine.bios_file);
	}
	machine->machine.bios_file = bios;

	return 0;
}
LUA_FUNCTION(RVMachine_PutKernel)
{
	RVMachine** pp = LUA->GetUserType<RVMachine*>(1, s_RVMachine_TypeId);
	if(!pp || !*pp)
	{
		LUA->ThrowError("Invalid RVMachine userdata");
		return 0;
	}
	RVMachine* machine = *pp;

	std::string path = LUA->CheckString(2);

	FILE* kernel = openFileInData(path);
	if(!kernel)
	{
		LUA->ThrowError("Invalid file!");
		return 0;
	}
	if(machine->machine.kernel_file)
	{
		fclose(machine->machine.kernel_file);
	}
	machine->machine.kernel_file = kernel;

	return 0;
}
LUA_FUNCTION(RVMachine_PutImage)
{
	RVMachine** pp = LUA->GetUserType<RVMachine*>(1, s_RVMachine_TypeId);
	if(!pp || !*pp)
	{
		LUA->ThrowError("Invalid RVMachine userdata");
		return 0;
	}
	RVMachine* machine = *pp;

	std::string path = LUA->CheckString(2);

	FILE* image = openFileInData(path);
	if(!image)
	{
		LUA->ThrowError("Invalid file!");
		return 0;
	}
	if(machine->machine.image_file)
	{
		fclose(machine->machine.image_file);
	}
	machine->machine.image_file = image;

	return 0;
}
LUA_FUNCTION(RVMachine_PutCustomDTB)
{
	RVMachine** pp = LUA->GetUserType<RVMachine*>(1, s_RVMachine_TypeId);
	if(!pp || !*pp)
	{
		LUA->ThrowError("Invalid RVMachine userdata");
		return 0;
	}
	RVMachine* machine = *pp;

	std::string path = LUA->CheckString(2);

	FILE* dtb = openFileInData(path);
	if(!dtb)
	{
		LUA->ThrowError("Invalid file!");
		return 0;
	}
	if(machine->machine.dtb_file)
	{
		fclose(machine->machine.dtb_file);
	}
	machine->machine.dtb_file = dtb;

	return 0;
}

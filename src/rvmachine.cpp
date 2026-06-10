#include "rvmachine.hpp"
#include "../include/GarrysMod/Lua/Interface.h"
#include "limits.h"
#include <algorithm>
#include <cstdio>
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
int s_RVMachine_TypeId						 = 0;
std::vector<RVMachine*> s_RVMachine_Instance = {};
LUA_FUNCTION(Create_RVMachine)
{
	uint64_t memsize   = (uint64_t)LUA->CheckNumber(1);
	uint8_t hart_count = (uint8_t)LUA->CheckNumber(2);

	RVMachine* machine = new RVMachine(memsize, hart_count);
	s_RVMachine_Instance.push_back(machine);
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

	if(machine->uart)
	{
		machine->uart = nullptr;
	}
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
	if(machine->machine.dtb_file)
	{
		fclose(machine->machine.dtb_file);
	}
	machine->machine.stop();
idle:
	if(machine->machine.work_thread_w) goto idle;
	s_RVMachine_Instance.erase(std::remove(s_RVMachine_Instance.begin(), s_RVMachine_Instance.end(), machine), s_RVMachine_Instance.end());
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
	snprintf(fpath, sizeof(fpath), "%s\\garrysmod\\data\\%s", currentDir, path.c_str());

	FILE* file = fopen(fpath, "rb");
	return file;
}
FILE* openFileInDataForWrite(std::string path)
{
	char currentDir[PATH_MAX];
	getcwd(currentDir, PATH_MAX);

	char fpath[PATH_MAX];
	snprintf(fpath, sizeof(fpath), "%s\\garrysmod\\data\\%s", currentDir, path.c_str());

	FILE* file = fopen(fpath, "wb+");
	return file;
}
FILE* openFileInDataForAppend(std::string path)
{
	char currentDir[PATH_MAX];
	getcwd(currentDir, PATH_MAX);

	char fpath[PATH_MAX];
	snprintf(fpath, sizeof(fpath), "%s\\garrysmod\\data\\%s", currentDir, path.c_str());

	FILE* file = fopen(fpath, "r+b");
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
	FILE* bios		 = openFileInData(path);
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

	FILE* image = openFileInDataForAppend(path);
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
LUA_FUNCTION(RVMachine_SetBootArgs)
{
	RVMachine** pp = LUA->GetUserType<RVMachine*>(1, s_RVMachine_TypeId);
	if(!pp || !*pp)
	{
		LUA->ThrowError("Invalid RVMachine userdata");
		return 0;
	}
	RVMachine* machine = *pp;

	std::string str			= LUA->CheckString(2);
	machine->machine.append = str;
	return 0;
}
LUA_FUNCTION(RVMachine_InitAutoDevices)
{
	RVMachine** pp = LUA->GetUserType<RVMachine*>(1, s_RVMachine_TypeId);
	if(!pp || !*pp)
	{
		LUA->ThrowError("Invalid RVMachine userdata");
		return 0;
	}
	RVMachine* machine = *pp;

	machine->machine.uart_out = openFileInDataForWrite("riscv/uart_output");
	machine->machine.init_auto_devices();
	machine->uart = machine->machine.mmio->get<UART>();
	return 0;
}
LUA_FUNCTION(RVMachine_InitFDT)
{
	RVMachine** pp = LUA->GetUserType<RVMachine*>(1, s_RVMachine_TypeId);
	if(!pp || !*pp)
	{
		LUA->ThrowError("Invalid RVMachine userdata");
		return 0;
	}
	RVMachine* machine = *pp;
	machine->machine.init_fdt();
	return 0;
}
LUA_FUNCTION(RVMachine_WriteFDT)
{
	RVMachine** pp = LUA->GetUserType<RVMachine*>(1, s_RVMachine_TypeId);
	if(!pp || !*pp)
	{
		LUA->ThrowError("Invalid RVMachine userdata");
		return 0;
	}
	RVMachine* machine = *pp;
	machine->machine.write_fdt();
	return 0;
}
LUA_FUNCTION(RVMachine_Run)
{
	RVMachine** pp = LUA->GetUserType<RVMachine*>(1, s_RVMachine_TypeId);
	if(!pp || !*pp)
	{
		LUA->ThrowError("Invalid RVMachine userdata");
		return 0;
	}
	RVMachine* machine = *pp;

	machine->machine.run();
	return 0;
}
LUA_FUNCTION(RVMachine_Stop)
{
	RVMachine** pp = LUA->GetUserType<RVMachine*>(1, s_RVMachine_TypeId);
	if(!pp || !*pp)
	{
		LUA->ThrowError("Invalid RVMachine userdata");
		return 0;
	}
	RVMachine* machine = *pp;

	machine->machine.stop();
	return 0;
}
LUA_FUNCTION(RVMachine_Reboot)
{
	RVMachine** pp = LUA->GetUserType<RVMachine*>(1, s_RVMachine_TypeId);
	if(!pp || !*pp)
	{
		LUA->ThrowError("Invalid RVMachine userdata");
		return 0;
	}
	RVMachine* machine = *pp;

	machine->machine.reset();
	return 0;
}
LUA_FUNCTION(RVMachine_PutChar)
{
	RVMachine** pp = LUA->GetUserType<RVMachine*>(1, s_RVMachine_TypeId);
	if(!pp || !*pp)
	{
		LUA->ThrowError("Invalid RVMachine userdata");
		return 0;
	}
	RVMachine* machine = *pp;

	unsigned char ch = LUA->CheckNumber(2);

	if(machine->uart)
	{
		machine->uart->receive_byte(ch);
	}
	return 0;
}

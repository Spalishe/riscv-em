#include "../include/GarrysMod/Lua/Interface.h"
#include "rvmachine.hpp"

/*
		require "HelloWorld"
		print( TestFunction( 5, 17 ) )
*/

using namespace GarrysMod::Lua;

GMOD_MODULE_OPEN()
{
	s_RVMachine_TypeId = LUA->CreateMetaTable("RVMachine");
	LUA->PushCFunction(RVMachine_Index);
	LUA->SetField(-2, "__index");
	LUA->PushCFunction(Destroy_RVMachine);
	LUA->SetField(-2, "__gc");
	LUA->PushCFunction(RVMachine_ToString);
	LUA->SetField(-2, "__tostring");
	LUA->PushCFunction(RVMachine_GetState);
	LUA->SetField(-2, "GetState");
	LUA->PushCFunction(RVMachine_InitMmap);
	LUA->SetField(-2, "InitMmap");
	LUA->PushCFunction(RVMachine_PutFirmware);
	LUA->SetField(-2, "PutFirmware");
	LUA->PushCFunction(RVMachine_PutFirmware);
	LUA->SetField(-2, "PutKernel");
	LUA->PushCFunction(RVMachine_PutFirmware);
	LUA->SetField(-2, "PutImage");
	LUA->PushCFunction(RVMachine_PutCustomDTB);
	LUA->SetField(-2, "PutCustomDTB");
	LUA->PushCFunction(RVMachine_SetBootArgs);
	LUA->SetField(-2, "SetBootArgs");
	LUA->PushCFunction(RVMachine_InitAutoDevices);
	LUA->SetField(-2, "InitAutoDevices");
	LUA->PushCFunction(RVMachine_Run);
	LUA->SetField(-2, "Run");
	LUA->PushCFunction(RVMachine_Stop);
	LUA->SetField(-2, "Stop");
	LUA->PushCFunction(RVMachine_Reboot);
	LUA->SetField(-2, "Reboot");

	LUA->Pop();

	LUA->PushSpecial(SPECIAL_GLOB);
	LUA->GetField(-1, "rvmachine");
	if(!LUA->IsType(-1, Type::Table))
	{
		LUA->Pop(1);
		LUA->CreateTable();
	}

	LUA->PushCFunction(Create_RVMachine);
	LUA->SetField(-2, "Create");

	LUA->SetField(-2, "rvmachine");

	return 0;
}

GMOD_MODULE_CLOSE()
{
	return 0;
}

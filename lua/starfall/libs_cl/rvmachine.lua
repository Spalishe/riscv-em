require("riscv-em")

--- Library for interfacing with RISCV-EM machines
--- Note: UART output is always located in data/riscv/uart_output
-- @name rvmachine
-- @class library
-- @libtbl rvmachine_library
SF.RegisterLibrary("rvmachine")
SF.RegisterType("RVMachine", true, false, FindMetaTable("RVMachine"))
return function(instance)
	local checkluatype = SF.CheckLuaType
	local library = instance.Libraries.rvmachine
	local rv_methods = instance.Types.RVMachine
	local rvwrap = instance.Types.RVMachine.Wrap
	local rvunwrap = instance.Types.RVMachine.Unwrap

	--- Creates RVMachine object
	-- @param number memsize Memory size (in bytes)
	-- @param number hart_count Number of HART's
	-- @client
	-- @return RVMachine
	function library.create(memsize, hart_count)
		checkluatype(memsize, TYPE_NUMBER)
		checkluatype(hart_count, TYPE_NUMBER)
		memsize = memsize or 268435456
		hart_count = hart_count or 1

		local obj = _G.rvmachine.Create(memsize, hart_count)
		return rvwrap(obj)
	end

	--- Turns the RVMachine into a string.
	-- @client
	-- @return string String representing the RVMachine.
	function rv_methods:__tostring()
		local this = rvunwrap(self)
		return tostring(this)
	end

	--- Returns RVMachine run state.
	--- 0 = Off
	--- 1 = Halted
	--- 2 = Running
	--- 3 = Resetting
	-- @client
	-- @return number Number representing RVMachine run state.
	function rv_methods:getState()
		local this = rvunwrap(self)
		return this:GetState()
	end

	--- Initializes entire machine, you need to call it once
	-- @param string fw_path Path to firmware, relative to data
	-- @param string|nil kernel_path Path to kernel, relative to data, can be nil
	-- @param string|nil image_path Path to Image, relative to data, can be nil
	-- @param string|nil append Custom boot args, can be nil
	-- @client
	function rv_methods:init(fw_path, kernel_path, image_path, append)
		checkluatype(fw_path, TYPE_STRING)
		local this = rvunwrap(self)
		this:InitMmap()
		this:PutFirmware(fw_path)
		if kernel_path ~= nil then
			this:PutKernel(kernel_path)
		end
		if image_path ~= nil then
			this:PutImage(image_path)
		end
		if append ~= nil then
			this:SetBootArgs(append)
		end
		this:InitFDT()
		this:InitAutoDevices()
		this:WriteFDT()
	end

	--- Runs machine
	-- @client
	function rv_methods:run()
		local this = rvunwrap(self)
		this:Run()
	end
	--- Stops machine
	-- @client
	function rv_methods:stop()
		local this = rvunwrap(self)
		this:Stop()
	end
	--- Reboots machine
	-- @client
	function rv_methods:reboot()
		local this = rvunwrap(self)
		this:Reboot()
	end
	--- Puts character in UART RX
	-- @param number char ASCII character
	-- @client
	function rv_methods:putchar(char)
		checkluatype(char, TYPE_NUMBER)
		char = math.Clamp(char, 0, 255)
		local this = rvunwrap(self)
		this:PutChar(char)
	end
end


local c = require('cdb_cli')
local mod = {}

-- Полезные инструменты для Lua.
-- local penlight = require "pl.import_into" ()

local function set_hsr(s, name, slave_a, slave_b)
	local p = {
		operation = "merge",
		target = "/ietf-interfaces:interfaces/interface",
		value = {
			["interface"] = {
				{
                    ["type"] = "angtel-interfaces:hsr",
					["name"] = name,
					["angtel-hsr:hsr"] = {
						["slave-a"] = slave_a,
						["slave-b"] = slave_b
					}
				}
			}
		}
	}
	s:cdb_edit_config(p)
end

local function check_interfaces(s, args.if_name, args.slave_a, args.slave_b)
	local pretty = require("pl.pretty")
	local target = "/ietf-interfaces:interfaces/interface"
	local data1 = s:cdb_get(target)
	local data2 = c.strip_cdb_data(data)
	s:output(pretty.write(data1))
	s:output(pretty.write(data2))

	--for key, value if ipairs(data) do
	--	if value["type"] ~= "angtel-interfaces:hsr" then
	--		goto continue
	--	end

	--	local slave_a, slave_b
	--	local slave_target = ("/ietf-interfaces:interfaces" ..
	--				"/interface[name='%s']/angtel-hsr:hsr"):format(value["name"])
	--	slaves_data = s:cdb_get(target)
	--	slaves_data = c.strip_cdb_data(slaves_data)



	--	::continue::
	--end
	return 1
end

local function get_hsr(s, name)
	local data
	local slave_a, slave_b
	local target = ("/ietf-interfaces:interfaces" ..
					"/interface[name='%s']/angtel-hsr:hsr"):format(name)

	-- Получение данных из CDB.
	data = s:cdb_get(target)
	-- Получение только содержимого контейнера angtel-hsr:hsr.
	data = c.strip_cdb_data(data)

	-- Контроль структуры получаемых данных.
	-- s:output(penlight.pretty.write(data))

	if data[1] == nil then
		return nil, nil
	end

	slave_a = data[1]["slave-a"]
	slave_b = data[1]["slave-b"]

	return slave_a, slave_b
end

local hsr_commands = {
	-- Корневая команда.
	["hsr"] = {
		description = "Commands for HSR configuration.",
	},
	-- Подкоманды.
	["hsr interface"] = {
		description = "Sets HSR interface parameters.",
		usage_info = "IF-NAME SLAVE-A SLAVE-B",
		parameters = {
			{"IF-NAME", "Interface name."},
			{"SLAVE-A", "Slave interface A."},
			{"SLAVE-B", "Slave interface B."}
		},
		args = {
			{
				type = "positional",
				name = "if_name",
				vtype = "string",
				nvalues = "1",
				required = true
			},
			{
				type = "positional",
				name = "slave_a",
				vtype = "string",
				nvalues = "1",
				required = true
			},
			{
				type = "positional",
				name = "slave_b",
				vtype = "string",
				nvalues = "1",
				required = true
			}
		},
   
		handler = function(s, args)
			local check = check_interfaces(s, args.if_name, args.slave_a, args.slave_b)
			if check == 1 then
				set_hsr(s, args.if_name, args.slave_a, args.slave_b)
			end
		end
	}
}

local common_commands = {
	["show hsr"] = {
		description = "Displays HSR parameters.",
	},
	["show hsr interface"] = {
		description = "Displays HSR interface parameters.",
		usage_info = "IF-NAME",
		parameters = {
			{"IF-NAME", "Interface name."}
		},
		args = {
			{
				type = "positional",
				name = "if_name",
				vtype = "string",
				nvalues = "1",
				required = true
			}
		},
		handler = function(s, args)
			local slave_a, slave_b = get_hsr(s, args.if_name)
			if slave_a == nil or slave_b == nil then
				s:output("Failed to get HSR parameters.")
				return
			end
			s:output(("Slave-A: %s, Slave-B: %s"):format(slave_a, slave_b))
		end
	}
}

local groups = {
	-- Каталоги CLI-интерфейса в которые нужно перейти для активации команд.
	-- Например: system, logging и т.д.
	{
		path = "/",
		commands = hsr_commands
	},
	{
		path = "/common",
		commands = common_commands
	},
}

-- Поиск обработчика команд CLI.
function mod.find_handler(name, path, cmd_name, arg_name)
	return c.find_handler(groups, name, path, cmd_name, arg_name)
end

-- Инициализация модуля команд CLI.
function mod.init(cli)
	cli:register_commands(groups)
end

return mod

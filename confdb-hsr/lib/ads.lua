 
```lua
function mod.set_session_methods(s)
s.cdb_edit_config = function(s, params, timeout)
    return s.__session__:cdb_send("edit-config", params, timeout)
end
s.cdb_get = function(s, target, timeout)
    return s.__session__:cdb_send("get", {target = target}, timeout)
end
s.cdb_get_config = function(s, target, timeout)
    return s.__session__:cdb_send("get-config", {target = target}, timeout)
end
s.cdb_get_status = function(s, target, timeout)
    return s.__session__:cdb_send("get-status", {target = target}, timeout)
end
s.cdb_call = function(s, params, timeout)
    return s.__session__:cdb_send("call", params, timeout)
end
s.cdb_check = function(s, params, timeout)
    return s.__session__:cdb_send("check", params, timeout)
end
s.delete_interface = function(s, name, timeout)
    local t = ("/ietf-interfaces:interfaces/interface[name='%s']"):format(name)
    local params = {
    target = t,
    operation = "delete"
    }

    return s.__session__:cdb_send("edit", params, timeout)
end
s.delete_locatable_item = function(s, name, timeout)
    local t = ("/angtel-geo-location:locatable-items/"..
    "locatable-item[name='%s']"):format(name)
    local params = {
    target = t,
    operation = "delete"
    }

    return s.__session__:cdb_send("edit", params, timeout)
end
s.change_dir = function(s, path, modestr)
return s.__session__:change_dir(path, modestr)
end
s.close = function(s)
return s.__session__:close()
end
s.input = function(s, timeout, prompt)
return s.__session__:input(timeout, prompt)
end
s.output = function(s, text)
return s.__session__:output(text)
end
s.get_fd = function(s)
return s.__session__:get_fd()
end
s.show_help = function(s, name)
return s.__session__:show_help(name)
end
s.show_history = function(s)
return s.__session__:show_history()
end
end
``` 

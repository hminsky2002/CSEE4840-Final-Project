set_global_assignment -entity "foo" -library "lib_foo" -name IP_TOOL_NAME "altera_mult_add"
set_global_assignment -entity "foo" -library "lib_foo" -name IP_TOOL_VERSION "21.1"
set_global_assignment -entity "foo" -library "lib_foo" -name IP_TOOL_ENV "mwpim"
set_global_assignment -library "lib_foo" -name SPD_FILE [file join $::quartus(sip_path) "foo.spd"]

set_global_assignment -library "lib_foo" -name MISC_FILE [file join $::quartus(sip_path) "foo_sim/foo.v"]

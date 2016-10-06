#!/usr/bin/python3

import os
import sys
import importlib
import json
import time

ext_to_run = sys.argv[1]
cmd_pipe = sys.argv[2]
ctrl_pipe = sys.argv[3]

print("DEBUG Importing extension to run")
module = importlib.import_module(ext_to_run)
Extension = getattr(module, "Extension")
ext_instance = Extension()

print("DEBUG Opening command channel")
command = open(cmd_pipe, 'r')

print("DEBUG Opening response channel")
control = open(ctrl_pipe, 'w')

run = True
ext_instance = None
cmdJSON = ""

print("DEBUG Entering event loop")
while run is True:
	print("DEBUG Waiting for command")
	#while cmdJSON is "":
	#	time.sleep(1)
	cmdJSON = command.readline()
	print("DEBUG cmdJSON="+cmdJSON)
	cmdObj = json.loads(cmdJSON)
	
	if cmdObj["method"] == "QUIT":
		run = False
	else:
		if ext_instance is None:
			ext_instance = Extension()
#			print("ERROR Extension instance is None!")
#			print("DEBUG method was " + cmdObj["method"])
		
		to_invoke = getattr(ext_instance, cmdObj["method"])

		if "arg" in cmdObj and cmdObj["arg"] != "":
			result = to_invoke(cmdObj["arg"])
		else:
			result = to_invoke()
		
		if result is not None:
			control.write(json.dumps(result) + "\n")
			control.flush()
		else:
			control.write("\n")
			control.flush()
	command.close()
	command = open(cmd_pipe, 'r')

command.close()
control.close()

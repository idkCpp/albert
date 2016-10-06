#!/usr/bin/python3

import sys
import os
import json

opcode = sys.argv[1]

command = os.open("cmd", os.O_WRONLY)
control = open("ctrl", "r")

d = {
  "method": "",
  "arg": ""
}

if opcode == "INITIALIZE":
  d["method"] = "userinit"
elif opcode == "METADATA":
  d["method"] = "getMetaData"
elif opcode == "NAME":
  d["method"] = "name"
elif opcode == "FINALIZE":
  d["method"] = "finalize"
elif opcode == "SETUPSESSION":
  d["method"] = "setupSession"
elif opcode == "TEARDOWNSESSION":
  d["method"] = "teardownSession"
elif opcode == "QUERY":
  d["method"] = "query"
  d["arg"] = sys.argv[2]
elif opcode == "FALLBACKS":
  d["method"] = "fallbacks"
  d["arg"] = sys.argv[2]
else:
  print("ERROR opcode " + sys.argv[1] + " unknown")

j = json.dumps(d)
b = str.encode(j+"\n")

os.write(command, b)
os.close(command)

ret = control.readline()
if ret != "" and ret != "\n" and ret != "null\n":
  print(ret)

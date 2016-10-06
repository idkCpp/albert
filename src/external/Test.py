#!/usr/bin/python3

class Extension:

  def __init__(self):
    self.metadata = {
      'iid': 'org.albert.extension.external.v1',
      'id': 'org.albert.extension.external.v1.Test',
      'name': 'Test.py Extension',
      'version': 'v0.1',
      'author': 'Martin Buergmann',
      'dependencies': [],
      'providesMatches': True,
      'providesFallbacks': False,
      'runTriggeredOnly': False,
      'triggers': [],
    }
    self.myname = "Test.py Extension"

  def userinit(self):
    return

  def getMetaData(self):
    return self.metadata

  def name(self):
    return self.myname

  def finalize(self):
    return

  def setupSession(self):
    return

  def teardownSession(self):
    return

  def query(self, querystring):
    return [{
      "id":"extension.wide.unique.id",
      "name":"Echo Hallo Item",
      "description":"This item echos hallo.",
      "iconpath":"",
      "actions":[
	{"name":"Echo Hallo", "command":"echo", "arguments":["hallo"] }
        #{ "name": "Launch", "method": "launch", "args": querystring }
      ]
    }]

  def fallbacks(self, querystring):
    return {}

  def launch(self, launchstr):
    print("3...2...1...LAUNCH")
    return launchstr

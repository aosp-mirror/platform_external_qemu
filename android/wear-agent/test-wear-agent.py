#!/usr/bin/env python

# This python script mocks the adb host server and tests the wear-agent program.
#

import socket
import os, signal
import time
import subprocess

# localhost
gHost = ''
# Make it different from default adb host port (5037)
gPort = 5038
def wearChild():
    os.execv("wear-agent", ["wear-agent", "-p", str(gPort)])
    os._exit(0)

gListeningSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
gListeningSocket.bind((gHost, gPort))
gBacklog = 1
gListeningSocket.listen(gBacklog)

def adbMockChild():
    gConnectedSocket, gConnectedAddress = gListeningSocket.accept()
    print 'Connected by', gConnectedAddress
    gData = gConnectedSocket.recv(1024)
    if gData:
        print str(gData)
        gConnectedSocket.send("OKAY")
        aDeviceList = "emulator-5554 device\n"
        aMessage = '%04x' % len(aDeviceList) + aDeviceList
#aMessage = aDeviceList
        print "send message: " + aMessage
        gConnectedSocket.send(aMessage)
        time.sleep(1)
    gConnectedSocket.close()



"""
gChildren = []
gWearAgentPid = os.fork()
if 0 == gWearAgentPid:
    wearChild()
else:
    print "Started Wear Agent: " + str(gWearAgentPid)
    gChildren.append(gWearAgentPid)
"""


"""
gAdbMockPid = os.fork()
if 0 == gAdbMockPid:
    adbMockChild()
else:
    print "Started Adb Mocker"
    gChildren.append(gAdbMockPid)

os.kill(gWearAgentPid, signal.SIGKILL)
for aChild in gChildren: 
    os.waitpid(aChild,0)
"""

gWearAgentProc = subprocess.Popen(['wear-agent', '-p',  str(gPort)], stdout=subprocess.PIPE)
adbMockChild()
while True:
    out = gWearAgentProc.stdout.readline()
    if out == '' and gWearAgentProc.poll() != None:
        break;
    if out.find('FAIL') >= 0:
        print out
        break
    if out.find('SUCCESS') >= 0:
        print out
        break

adbMockChild()
while True:
    out = gWearAgentProc.stdout.readline()
    if out == '' and gWearAgentProc.poll() != None:
        break;
    if out.find('FAIL') >= 0:
        print out
        break
    if out.find('SUCCESS') >= 0:
        print out
        break
gWearAgentProc.terminate()

print "Wear Agent completes"
print "parent completes"









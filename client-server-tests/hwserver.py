#!/usr/bin/python

import zmq

context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind("tcp://*:5555")

while True:
    inMsg = socket.recv().decode('utf-8')
    outMsg = inMsg[::-1]
    print(outMsg)
    socket.send_string(outMsg)

#!/usr/bin/python

import zmq

context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:5555")
while True:
    outMsg = input()
    socket.send_string(outMsg)
    inMsg = socket.recv()
    print(inMsg.decode('utf-8'))

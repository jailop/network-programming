# Client-server examples

The first example reverses the input

The second example takes a command as argument and send messages to that command.

Example:

        $ ./test2 cat

The fist example works as argument for the second one:

        $ ./test2 ./test1

hwclient and hwserver is like the test1, but using zmq.

hwclient.py and hwserver.py is the same, but in Python

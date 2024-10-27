Echo client and server developed for my bachelor thesis in order to test two things: Throughput and latency of a servers performance. 

In order to run the program, the server is started first. The server is run from the terminal like any other C program, by navigating to the folder holding the program, then typing server.c -o server, then executing the program typing ./server
The client is started next. It is first build by typing client.c -o client, then executing the program. The client needs four arguments: the IP-address of the server, the frame size, the framerate and the runtime of the test. If testing latency, the program needs to run longer than 70 seconds in its current state.

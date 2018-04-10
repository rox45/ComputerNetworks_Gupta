#!/bin/bash
# For running test on client and server file

# Build 
make -B client server

# Open new terminal and run server
osascript -e 'tell app "Terminal" 
	do script "cd /Users/ryota/Documents/GitHub/ComputerNetworksProject && ./server 9000" 
end tell'

#sleep one second
sleep 1

# Open new terminal and run client
osascript -e 'tell application "Terminal" 
	do script "cd /Users/ryota/Documents/GitHub/ComputerNetworksProject && printf \"test file successful\" | ./client 127.0.0.1 9000" 
end tell'

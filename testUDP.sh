#!/bin/bash
# For running test on client and server file for UDP

# Build
make -B clientUDP serverUDP

# Open new terminal and run server
#osascript -e 'tell app "Terminal"
        do script "cd /Users/ryota/Documents/GitHub/ComputerNetworksProject/serverBinaries && ./serverUDP"
end tell'

#sleep one second
sleep 1

# Open new terminal and run client
#osascript -e 'tell application "Terminal"
        do script "cd /Users/ryota/Documents/GitHub/ComputerNetworksProject/clientBinaries && printf \"file.txt\" | ./clientUDP"
end tell'


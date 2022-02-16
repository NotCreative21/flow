build: 
	gcc -Wall src/flow.c -O2 -o flow -lxcb -lxcb-util
display:
	killall Xephyr || Xephyr -screen 1280x720 :0 &
test:
	kill -9 $(pidof flow) || make build
	DISPLAY=:0 ./flow &

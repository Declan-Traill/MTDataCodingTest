Packages to install:

sudo apt-get update
sudo apt-get install libjsoncpp-dev
sudo apt-get install libboost-all-dev
sudo apt-get update
sudo apt-get install libboost-all-dev

==================================================

Command to build scales_parser:

make -f ./make.mk all

==================================================

Commands to install serial for Python:

wget https://bootstrap.pypa.io/pip/2.7/get-pip.py
python2.7 get-pip.py
python2.7 -m pip install pyserial

==================================================

Commands to set up the virtual Comm port link:

sudo apt install socat
sudo socat PTY,raw,echo=0,link=/dev/ttyP0 PTY,raw,echo=0,link=/dev/ttyP1 &

==================================================

Change to simulator .cfg file:

dev = /dev/ttyP1            ; The serial device connected to the computer

==================================================

Command to run the Simulator:

sudo python2.7 ./pacific_scales_simulator.py

==================================================

Command to run the scales_parser:

sudo ./scales_parser /dev/ttyP0




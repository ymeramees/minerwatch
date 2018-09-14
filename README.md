# MinerWatch

MinerWatch is a small program to build a system which checks constantly if all computers are up and responsive. One instance is run as a server and other(s) as client(s). Clients constantly send a keep-alive signal (can also be done with bash script) to the server and server keeps track, when was last signal received for each client. If any of the clients fails to send the signal for specified time, it will be hard-reset by Arduino Nano.

All computers' reset buttons are wired to the Arduino Nano. Arduino Nano is connected to the computer on which the server is running and is listening for the reset commands and keep-alive signals from the server. If server becomes unresponsive, Nano will reset the server machine. 

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

Only Qt framework is needed, Qt 5.10.0 has been used.

### Installing/Deployment

Just build with Qt Creator and copy executable together with required Qt files.

## Built With

* [Qt](http://www.qt-project.org)

## License

This project is licensed under the DBAD License - see the [LICENSE.md](LICENSE.md) file for details

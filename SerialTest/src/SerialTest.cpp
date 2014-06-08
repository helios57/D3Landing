//============================================================================
//Name:SerialTest.cpp
//Author:
//Version:
//Copyright:Yourcopyrightnotice
//Description:HelloWorldinC++,Ansi-style
//============================================================================

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

using namespace std;

//Opened port!
//Link ttyACM0 is connected.
//
//NuttShell (NSH)
//nsh>
//nsh> sh /etc/init.d/rc.usb
//Starting MAVLink on this USB console
//[uorb] already loaded
//uORB started
//
//mavlink: .
//mavlink: terminated.
//stopped other MAVLink instance
//mavlink: MAVLink v1.0 serial interface starting...
//mavlink: DOWNLINK MODE
//mavlink: UART is /dev/ttyACM0, baudrate is 230400
//
//mavlink_uart_rcv: No waypoints send
//mavlink_uart_rcv: Sent waypoint count (0) to ID 255
//commander: .
//commander: terminated.
//Commander stopped
//
//(MAV 001:50) [mavlink pm] sending list
//(MAV 001:50) [cmd] started
//(MAV 001:50) [cmd] arming state: STANDBY
//

int main(int argc, char** argv) {
	struct termios tio;
	struct termios stdio;
	struct termios old_stdio;
	int tty_fd;

	unsigned char c = 'D';
	tcgetattr(STDOUT_FILENO, &old_stdio);

	memset(&stdio, 0, sizeof(stdio));
	stdio.c_iflag = 0;
	stdio.c_oflag = 0;
	stdio.c_cflag = 0;
	stdio.c_lflag = 0;
	stdio.c_cc[VMIN] = 1;
	stdio.c_cc[VTIME] = 0;
	tcsetattr(STDOUT_FILENO, TCSANOW, &stdio);
	tcsetattr(STDOUT_FILENO, TCSAFLUSH, &stdio);
	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);     // make the reads non-blocking

	memset(&tio, 0, sizeof(tio));
	tio.c_iflag = 0;
	tio.c_oflag = 0;
	tio.c_cflag = CS8 | CREAD | CLOCAL; // 8n1, see termios.h for more information
	tio.c_lflag = 0;
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 5;

	tty_fd = open("/dev/ttyACM0", O_RDWR | O_NONBLOCK);
	cfsetospeed(&tio, B115200);            // 115200 baud
	cfsetispeed(&tio, B115200);            // 115200 baud

	tcsetattr(tty_fd, TCSANOW, &tio);

	char cmd[] = "sh /etc/init.d/rc.usb";
	for (char cc : cmd) {
		c = cc;
		write(tty_fd, &c, 1);
	}
	while (c != 'q') {
		if (read(tty_fd, &c, 1) > 0)
			write(STDOUT_FILENO, &c, 1); // if new data is available on the serial port, print it out
		if (read(STDIN_FILENO, &c, 1) > 0)
			write(tty_fd, &c, 1); // if new data is available on the console, send it to the serial port
	}

	close(tty_fd);
	tcsetattr(STDOUT_FILENO, TCSANOW, &old_stdio);

	tty_fd = open("/dev/ttyACM0", O_RDWR | O_NONBLOCK);
	cfsetospeed(&tio, B230400);            // 115200 baud
	cfsetispeed(&tio, B230400);            // 115200 baud
	tcsetattr(tty_fd, TCSANOW, &tio);
	while (c != 'q' ||true) {
		if (read(tty_fd, &c, 1) > 0)
			write(STDOUT_FILENO, &c, 1); // if new data is available on the serial port, print it out
		if (read(STDIN_FILENO, &c, 1) > 0)
			write(tty_fd, &c, 1); // if new data is available on the console, send it to the serial port
	}

	close(tty_fd);
	tcsetattr(STDOUT_FILENO, TCSANOW, &old_stdio);
	return EXIT_SUCCESS;
}

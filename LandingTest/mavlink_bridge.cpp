#include "mavlink_bridge.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

uint8_t buf[MAVLINK_MAX_PACKET_LEN];
mavlink_message_t msg;
mavlink_status_t status;
mavlink_set_position_target_local_ned_t setpointAdjustment;
struct termios bridge_tio;
struct termios bridge_stdio;
struct termios bridge_old_stdio;
int bridge_tty_fd = 0;
unsigned char bridge_c = 'D';


using namespace std;

void close(int tty_fd, const struct termios& old_stdio) {
	close(tty_fd);
	tcsetattr(STDOUT_FILENO, TCSANOW, &old_stdio);
	exit(1);
}

void initStreams() {
	memset(&setpointAdjustment, 0, sizeof(mavlink_set_position_target_local_ned_t));

	if (tcgetattr(STDOUT_FILENO, &bridge_old_stdio) != 0) {
		printf("tcgetattr(STDOUT_FILENO, &old_stdio)!= 0");
		close(bridge_tty_fd, bridge_old_stdio);
	}
	memset(&bridge_stdio, 0, sizeof(bridge_stdio));
	bridge_stdio.c_iflag = 0;
	bridge_stdio.c_oflag = 0;
	bridge_stdio.c_cflag = 0;
	bridge_stdio.c_lflag = 0;
	bridge_stdio.c_cc[VMIN] = 1;
	bridge_stdio.c_cc[VTIME] = 0;
	if (tcsetattr(STDOUT_FILENO, TCSANOW, &bridge_stdio) != 0) {
		printf("tcsetattr(STDOUT_FILENO, TCSANOW, &stdio)!= 0");
		close(bridge_tty_fd, bridge_old_stdio);
	}
	//Check if working
	tcsetattr(STDOUT_FILENO, TCSAFLUSH, &bridge_stdio);
	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
	memset(&bridge_tio, 0, sizeof(bridge_tio));
	bridge_tio.c_iflag = 0;
	bridge_tio.c_oflag = 0;
	bridge_tio.c_cflag = CS8 | CREAD | CLOCAL;
	bridge_tio.c_lflag = 0;
	bridge_tio.c_cc[VMIN] = 1;
	bridge_tio.c_cc[VTIME] = 5;
	bridge_tty_fd = open("/dev/ttyACM0", O_RDWR | O_NONBLOCK);

	if (bridge_tty_fd == 0) {
		printf("open(\"/dev/ttyACM0\", O_RDWR | O_NONBLOCK)==0");
		close(bridge_tty_fd, bridge_old_stdio);
	}
	if (cfsetospeed(&bridge_tio, B57600) != 0) { // 115200 baud = B115200
		printf("cfsetospeed(&tio, B115200)!= 0");
		close(bridge_tty_fd, bridge_old_stdio);
	}
	if (cfsetispeed(&bridge_tio, B57600) != 0) { // 115200 baud = B115200
		printf("cfsetispeed(&tio, B115200)!= 0");
		close(bridge_tty_fd, bridge_old_stdio);
	}
	if (tcsetattr(bridge_tty_fd, TCSANOW, &bridge_tio) != 0) {
		printf("tcsetattr(tty_fd, TCSANOW, &tio)!= 0");
		close(bridge_tty_fd, bridge_old_stdio);
	}
	char cmd[] = "sh /etc/init.d/rc.usb\n";
	sleep(2);
	while (read(bridge_tty_fd, &bridge_c, 1) > 0 && bridge_c != 0) {
		write(STDOUT_FILENO, &bridge_c, 1); // if new data is available on the serial port, print it out
	}
	write(bridge_tty_fd, cmd, sizeof(cmd));
	sleep(2);
	while (read(bridge_tty_fd, &bridge_c, 1) > 0 && bridge_c != 0) {
		write(STDOUT_FILENO, &bridge_c, 1); // if new data is available on the serial port, print it out
	}
}

void readFromStream() {
	int counter = 0;
	while (read(bridge_tty_fd, &bridge_c, 1) > 0 && counter++ < 1000) {
		if (mavlink_parse_char(MAVLINK_COMM_0, bridge_c, &msg, &status)) {
			if (msg.msgid == MAVLINK_MSG_ID_STATUSTEXT) {
				mavlink_statustext_t s;
				mavlink_msg_statustext_decode(&msg, &s);
				printf("status %s\n", s.text);
			}
			if (msg.msgid == MAVLINK_MSG_ID_HIGHRES_IMU) {
				mavlink_highres_imu_t hr;
				mavlink_msg_highres_imu_decode(&msg, &hr);
				printf("highres imu: time=%f press=%f temp=%f acc={%f,%f,%f} gyro={%f,%f,%f} mag={%f,%f,%f}\n", //
						(float) hr.time_usec, hr.abs_pressure, hr.temperature, hr.xacc, hr.yacc, hr.zacc, hr.xgyro, hr.ygyro, hr.zgyro, hr.xmag, hr.ymag, hr.zmag);	//
			}
		}
	}
}

uint16_t sendMessage() {
	mavlink_msg_set_position_target_local_ned_encode(57, 57, &msg, &setpointAdjustment);
	uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
	write(bridge_tty_fd, &buf, len);
	return len;
}

void closeStream() {
	close(bridge_tty_fd, bridge_old_stdio);
}

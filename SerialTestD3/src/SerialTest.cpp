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
#include "mavlink/common/mavlink.h"

using namespace std;

void close(int tty_fd, const struct termios& old_stdio) {
	close(tty_fd);
	tcsetattr(STDOUT_FILENO, TCSANOW, &old_stdio);
}

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

//<message id="56" name="SET_ROLL_PITCH_YAW_THRUST">
//            <description>Set roll, pitch and yaw.</description>
//            <field type="uint8_t" name="target_system">System ID</field>
//            <field type="uint8_t" name="target_component">Component ID</field>
//            <field type="float" name="roll">Desired roll angle in radians</field>
//            <field type="float" name="pitch">Desired pitch angle in radians</field>
//            <field type="float" name="yaw">Desired yaw angle in radians</field>
//            <field type="float" name="thrust">Collective thrust, normalized to 0 .. 1</field>
//</message>

// Example variable, by declaring them static they're persistent
// and will thus track the system state

int main(int argc, char** argv) {
	struct termios tio;
	struct termios stdio;
	struct termios old_stdio;
	int tty_fd;

	unsigned char c = 'D';
	if (tcgetattr(STDOUT_FILENO, &old_stdio) != 0) {
		printf("tcgetattr(STDOUT_FILENO, &old_stdio)!= 0");
		close(tty_fd, old_stdio);
		return 1;
	}

	memset(&stdio, 0, sizeof(stdio));
	stdio.c_iflag = 0;
	stdio.c_oflag = 0;
	stdio.c_cflag = 0;
	stdio.c_lflag = 0;
	stdio.c_cc[VMIN] = 1;
	stdio.c_cc[VTIME] = 0;
	if (tcsetattr(STDOUT_FILENO, TCSANOW, &stdio) != 0) {
		printf("tcsetattr(STDOUT_FILENO, TCSANOW, &stdio)!= 0");
		close(tty_fd, old_stdio);
		return 1;
	}
	//Check if working
	tcsetattr(STDOUT_FILENO, TCSAFLUSH, &stdio);
	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

	memset(&tio, 0, sizeof(tio));
	tio.c_iflag = 0;
	tio.c_oflag = 0;
	tio.c_cflag = CS8 | CREAD | CLOCAL; // 8n1, see termios.h for more information
	tio.c_lflag = 0;
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 5;

	tty_fd = open("/dev/ttyACM0", O_RDWR | O_NONBLOCK);
	if (tty_fd == 0) {
		printf("open(\"/dev/ttyACM0\", O_RDWR | O_NONBLOCK)==0");
		close(tty_fd, old_stdio);
		return 1;
	}

	if (cfsetospeed(&tio, B115200) != 0) { // 115200 baud
		printf("cfsetospeed(&tio, B115200)!= 0");
		close(tty_fd, old_stdio);
		return 1;
	}

	if (cfsetispeed(&tio, B115200) != 0) { // 115200 baud
		printf("cfsetispeed(&tio, B115200)!= 0");
		close(tty_fd, old_stdio);
		return 1;
	}

	if (tcsetattr(tty_fd, TCSANOW, &tio) != 0) {
		printf("tcsetattr(tty_fd, TCSANOW, &tio)!= 0");
		close(tty_fd, old_stdio);
		return 1;
	}

	char cmd[] = "sh /etc/init.d/rc.usb\n";
	sleep(2);
	while (read(tty_fd, &c, 1) > 0) {
		write(STDOUT_FILENO, &c, 1); // if new data is available on the serial port, print it out
	}
	write(tty_fd, cmd, sizeof(cmd));
	sleep(2);
	while (read(tty_fd, &c, 1) > 0) {
		write(STDOUT_FILENO, &c, 1); // if new data is available on the serial port, print it out
	}

	mavlink_message_t msg;
	mavlink_status_t status;
	while (true) {
		if (read(tty_fd, &c, 1) > 0) {
			if (mavlink_parse_char(MAVLINK_COMM_0, c, &msg, &status)) {

				// Handle message
				switch (msg.msgid) {
				case MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_THRUST: {
					mavlink_set_roll_pitch_yaw_thrust_t setpoint;
					mavlink_msg_set_roll_pitch_yaw_thrust_decode(&msg, &setpoint);
					break;
				}
				case MAVLINK_MSG_ID_HEARTBEAT: {
					mavlink_heartbeat_t hb;
					mavlink_msg_heartbeat_decode(&msg, &hb);
					// E.g. read GCS heartbeat and go into
					// comm lost mode if timer times out
					//printf("mavlink_heartbeat_t\n");
					break;
				}
				case MAVLINK_MSG_ID_SYS_STATUS: {
					mavlink_sys_status_t s;
					mavlink_msg_sys_status_decode(&msg, &s);
					//printf("mavlink_sys_status_t\n");
					break;
				}
				case MAVLINK_MSG_ID_STATUSTEXT: {
					mavlink_statustext_t s;
					mavlink_msg_statustext_decode(&msg, &s);
					printf("status %s\n", s.text);
					break;
				}
				case MAV_COMPONENT_ENUM_END: {
					//check
					break;
				}
				case MAVLINK_MSG_ID_OPTICAL_FLOW: {
					mavlink_optical_flow_t of;
					mavlink_msg_optical_flow_decode(&msg, &of);
					//printf("optical flow: f_x=%i f_y=%i\n",					//
					//		(int) of.flow_x, (int) of.flow_y);				//
					break;
				}
				case MAVLINK_MSG_ID_HIGHRES_IMU: {
					mavlink_highres_imu_t hr;
					mavlink_msg_highres_imu_decode(&msg, &hr);
					printf("highres imu: time=%f press=%f temp=%f acc={%f,%f,%f} gyro={%f,%f,%f} mag={%f,%f,%f}\n",					//
							(float) hr.time_usec, hr.abs_pressure, hr.temperature, hr.xacc, hr.yacc, hr.zacc, hr.xgyro, hr.ygyro, hr.zgyro, hr.xmag, hr.ymag, hr.zmag);	//
					break;
				}
				case MAVLINK_MSG_ID_GPS_RAW_INT: {
					mavlink_gps_raw_int_t gpsri;
					mavlink_msg_gps_raw_int_decode(&msg, &gpsri);
					//printf("mavlink_gps_raw_int_t\n");
					break;
				}
				case MAVLINK_MSG_ID_ATTITUDE: {
					mavlink_attitude_t a;
					mavlink_msg_attitude_decode(&msg, &a);
					//printf("mavlink_attitude_t\n");
					break;
				}
				case MAVLINK_MSG_ID_LOCAL_POSITION_NED: {
					mavlink_local_position_ned_t ned;
					mavlink_msg_local_position_ned_decode(&msg, &ned);
					//printf("mavlink_local_position_ned_t\n");
					break;
				}
				case MAVLINK_MSG_ID_SERVO_OUTPUT_RAW: {
					mavlink_servo_output_raw_t s;
					mavlink_msg_servo_output_raw_decode(&msg, &s);
					//printf("mavlink_servo_output_raw_t\n");
					break;
				}
				case MAVLINK_MSG_ID_ROLL_PITCH_YAW_THRUST_SETPOINT: {
					mavlink_roll_pitch_yaw_thrust_setpoint_t s;
					mavlink_msg_roll_pitch_yaw_thrust_setpoint_decode(&msg, &s);
					//printf("mavlink_roll_pitch_yaw_thrust_setpoint_t\n");
					break;
				}
				case MAVLINK_MSG_ID_VFR_HUD: {
					mavlink_vfr_hud_t h;
					mavlink_msg_vfr_hud_decode(&msg, &h);
					//printf("mavlink_vfr_hud_t\n");
					break;
				}
				case MAVLINK_MSG_ID_COMMAND_LONG:
					// EXECUTE ACTION
					break;
				default:
					printf("unbekannte msg mit id %u \n", (unsigned int) msg.msgid);
					//Do nothing
					break;
				}
				//c = msg.sysid;
				//write(STDOUT_FILENO, &c, 1); // if new data is available on the serial port, print it out
			}

		}
		if (read(STDIN_FILENO, &c, 1) > 0)
			break; // if new data is available on the console, send it to the serial port
	}

	close(tty_fd, old_stdio);
	return EXIT_SUCCESS;
}

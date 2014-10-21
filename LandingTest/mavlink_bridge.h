#ifndef MAVLINK_BRIDGE_H
#define MAVLINK_BRIDGE_H

#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include "mavlink/common/mavlink.h"

void closeStream();
void close(int tty_fd, const struct termios& old_stdio);
void initStreams();
void readFromStream();
uint16_t sendMessage();

#endif //MAVLINK_BRIDGE_H

#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "spidriver.h"

// ****************************   Serial port  ********************************

int openSerialPort(const char *portname)
{
  int fd = open(portname, O_RDWR | O_NOCTTY);
  if (fd == -1)
    perror(portname);

  struct termios Settings;

  tcgetattr(fd, &Settings);

  cfsetispeed(&Settings, B460800);
  cfsetospeed(&Settings, B460800);

  Settings.c_cflag &= ~PARENB;
  Settings.c_cflag &= ~CSTOPB;
  Settings.c_cflag &= ~CSIZE;
  Settings.c_cflag &= ~CRTSCTS;
  Settings.c_cflag |=  CS8;
  Settings.c_cflag |=  CREAD | CLOCAL;

  Settings.c_iflag &= ~(IXON | IXOFF | IXANY);
  Settings.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);

  Settings.c_oflag &= ~OPOST;

  Settings.c_cc[VMIN] = 1;

  if (tcsetattr(fd, TCSANOW, &Settings) != 0)
    perror("Serial settings");

  return fd;
}

size_t readFromSerialPort(int fd, char *b, size_t s)
{
  size_t n, t;
  t = 0;
  while (t < s) {
    n = read(fd, b + t, s);
    t += n;
  }
#ifdef VERBOSE
  printf(" READ %d %d: ", (int)s, int(n));
  for (int i = 0; i < s; i++)
    printf("%02x ", 0xff & b[i]);
  printf("\n");
#endif
  return s;
}

void writeToSerialPort(int fd, const char *b, size_t s)
{
  write(fd, b, s);
#ifdef VERBOSE
  printf("WRITE %u: ", (int)s);
  for (int i = 0; i < s; i++)
    printf("%02x ", 0xff & b[i]);
  printf("\n");
#endif
}

// ******************************  SPIDriver  *********************************

void spi_connect(SPIDriver *sd, const char* portname)
{
  sd->port = openSerialPort(portname);
  writeToSerialPort(sd->port, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@", 64);
  for (int i = 0; i < 256; i++) {
    char tx[2] = {'e', i};
    writeToSerialPort(sd->port, tx, 2);
    char rx[1];
    readFromSerialPort(sd->port, rx, 1);
    if ((rx[0] & 0xff) != i)
      assert(0);
  }
  printf("\n");
  spi_getstatus(sd);
}

void spi_getstatus(SPIDriver *sd)
{
  char readbuffer[100];
  int bytesRead;

  writeToSerialPort(sd->port, "?", 1);
  bytesRead = readFromSerialPort(sd->port, readbuffer, 80);
  readbuffer[bytesRead] = 0;
  // printf("%d Bytes were read: %.*s\n", bytesRead, bytesRead, readbuffer);
  sscanf(readbuffer, "[%15s %8s %" SCNu64 " %f %f %f %d %d %d %x ]",
    sd->model,
    sd->serial,
    &sd->uptime,
    &sd->voltage_v,
    &sd->current_ma,
    &sd->temp_celsius,
    &sd->a,
    &sd->b,
    &sd->cs,
    &sd->debug
    );
}

void spi_sel(SPIDriver *sd)
{
  writeToSerialPort(sd->port, "s", 1);
  sd->cs = 0;
}

void spi_unsel(SPIDriver *sd)
{
  writeToSerialPort(sd->port, "u", 1);
  sd->cs = 1;
}

void spi_seta(SPIDriver *sd, char v)
{
  char cmd[2] = {'a', v};
  writeToSerialPort(sd->port, cmd, 2);
  sd->a = v;
}

void spi_setb(SPIDriver *sd, char v)
{
  char cmd[2] = {'b', v};
  writeToSerialPort(sd->port, cmd, 2);
  sd->b = v;
}

void spi_write(SPIDriver *sd, size_t nn, const char bytes[])
{
  for (size_t i = 0; i < nn; i += 64) {
    size_t len = ((nn - i) < 64) ? (nn - i) : 64;
    char cmd[65] = {(char)(0xc0 + len - 1)};
    memcpy(cmd + 1, bytes + i, len);
    writeToSerialPort(sd->port, cmd, 1 + len);
  }
}

void spi_read(SPIDriver *sd, size_t nn, char bytes[])
{
  for (size_t i = 0; i < nn; i += 64) {
    size_t len = ((nn - i) < 64) ? (nn - i) : 64;
    char cmd[65] = {(char)(0x80 + len - 1), 0};
    writeToSerialPort(sd->port, cmd, 1 + len);
    readFromSerialPort(sd->port, bytes + i, len);
  }
}

void spi_writeread(SPIDriver *sd, size_t nn, char bytes[])
{
  for (size_t i = 0; i < nn; i += 64) {
    size_t len = ((nn - i) < 64) ? (nn - i) : 64;
    char cmd[65] = {(char)(0x80 + len - 1)};
    memcpy(cmd + 1, bytes + i, len);
    writeToSerialPort(sd->port, cmd, 1 + len);
    readFromSerialPort(sd->port, bytes + i, len);
  }
}


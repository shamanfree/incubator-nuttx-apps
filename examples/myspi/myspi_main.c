#include <nuttx/config.h>

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>

#include <nuttx/spi/spi.h>
#include <nuttx/spi/spi_transfer.h>

#define SPIIOC_TRANSFER _SPIIOC(0x0001)

#define MAX_XDATA 40
#define CS 0
#define DELAY 0
#define COUNT 1

int main(int argc, FAR char *argv[]) {
  uint8_t txdata[MAX_XDATA] = {0xDE, 0xAD, 0xBE, 0xEF};
  uint8_t rxdata[MAX_XDATA] = {0};

  int ret;
  int fd;
  struct spi_trans_s trans;
  struct spi_sequence_s seq;

  fd = open("/dev/spi0", O_RDONLY);
  if (fd < 0) {
    return ERROR;
  }

  seq.dev = SPIDEV_ID(SPIDEVTYPE_USER, CS);
  seq.mode = CONFIG_MYSPITOOL_DEFMODE;
  seq.nbits = CONFIG_MYSPITOOL_DEFWIDTH;
  seq.frequency = CONFIG_MYSPITOOL_DEFFREQ;
  seq.ntrans = 1;
  seq.trans = &trans;

  trans.deselect = true;
  trans.delay = DELAY;
  trans.nwords = COUNT;
  trans.txbuffer = txdata;
  trans.rxbuffer = rxdata;

  ret = ioctl(fd, SPIIOC_TRANSFER, seq);
  close(fd);

  return OK;
}

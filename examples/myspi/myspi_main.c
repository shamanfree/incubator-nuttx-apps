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

#define ROWS_A 2
#define COLUMNS_A 3
#define COLUMNS_B 4

int main(int argc, FAR char *argv[]) {
  int ret;
  int fd;
  struct spi_trans_s trans;
  struct spi_sequence_s seq;

  uint8_t txdata[MAX_XDATA] = {0xDE, 0xAD, 0xBE, 0xEF};
  uint8_t rxdata[MAX_XDATA] = {0};

  uint8_t matrix_a[ROWS_A][COLUMNS_A];
  uint8_t matrix_b[COLUMNS_A][COLUMNS_B];

  uint64_t matrix_c[ROWS_A][COLUMNS_B];

  for (uint8_t i = 0; i < ROWS_A; i++) {
    for (uint8_t j = 0; j < COLUMNS_A; j++) {
      matrix_a[i][j] = rand() % 0xFF;
    }
  }

  for (uint8_t i = 0; i < ROWS_A; i++) {
    for (uint8_t j = 0; j < COLUMNS_A; j++) {
      printf("%d ", matrix_a[i][j]);
    }
    printf("\n");
  }

  printf("\n");

  for (uint8_t i = 0; i < COLUMNS_A; i++) {
    for (uint8_t j = 0; j < COLUMNS_B; j++) {
      matrix_b[i][j] = rand() % 0xFF;
    }
  }

  for (uint8_t i = 0; i < COLUMNS_A; i++) {
    for (uint8_t j = 0; j < COLUMNS_B; j++) {
      printf("%d ", matrix_b[i][j]);
    }
    printf("\n");
  }

  printf("\n");

  for (int i = 0; i < ROWS_A; i++) {
    for (int j = 0; j < COLUMNS_B; j++) {
      matrix_c[i][j] = 0;
      for (int k = 0; k < COLUMNS_A; k++)
        matrix_c[i][j] += matrix_a[i][k] * matrix_b[k][j];
    }
  }

  for (uint8_t i = 0; i < ROWS_A; i++) {
    for (uint8_t j = 0; j < COLUMNS_B; j++) {
      printf("%d ", matrix_c[i][j]);
    }
    printf("\n");
  }

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

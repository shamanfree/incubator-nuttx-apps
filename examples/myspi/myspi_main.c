#include <nuttx/config.h>

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>

#include <nuttx/spi/spi.h>
#include <nuttx/spi/spi_transfer.h>

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SPIIOC_TRANSFER _SPIIOC(0x0001)

#define MAX_XDATA 0x100
#define CS 0
#define DELAY 0
#define COUNT 1

#define ROWS_A 2
#define COLUMNS_A 3
#define COLUMNS_B 4
#define COUNTER_MAX 22

static pthread_mutex_t mut;
static volatile int my_mutex = 0;
static unsigned long nloops[2] = {0, 0};
static unsigned long nerrors[2] = {0, 0};
static volatile bool bendoftest;

uint8_t matrix_a[ROWS_A][COLUMNS_A];
uint8_t matrix_b[COLUMNS_A][COLUMNS_B];

uint64_t matrix_c[ROWS_A][COLUMNS_B];

struct spi_trans_s trans;
struct spi_sequence_s seq;
int fd;
uint8_t txdata[MAX_XDATA] = {0xDE, 0xAD, 0xBE, 0xEF};
uint8_t rxdata[MAX_XDATA] = {0};

void matrix_calculation_thread(void *parameter) {
  int my_id = (int)parameter;
  int my_ndx = my_id - 1;
  int cnt;
  while (!bendoftest) {

    for (uint8_t i = 0; i < ROWS_A; i++) {
      for (uint8_t j = 0; j < COLUMNS_A; j++) {
        matrix_a[i][j] = rand() % 0xFF;
      }
    }

    for (uint8_t i = 0; i < COLUMNS_A; i++) {
      for (uint8_t j = 0; j < COLUMNS_B; j++) {
        matrix_b[i][j] = rand() % 0xFF;
      }
    }

    printf("LOCK mutex by thread1\n");
    if ((pthread_mutex_lock(&mut)) != 0) {
      printf("ERROR thread %d: pthread_mutex_lock failed\n", my_id);
    }

    for (int i = 0; i < ROWS_A; i++) {
      for (int j = 0; j < COLUMNS_B; j++) {
        matrix_c[i][j] = 0;
        for (int k = 0; k < COLUMNS_A; k++)
          matrix_c[i][j] += matrix_a[i][k] * matrix_b[k][j];
      }
    }
    printf("MATRIX 0 %d= \n", cnt++);
    for (uint8_t i = 0; i < ROWS_A; i++) {
      for (uint8_t j = 0; j < COLUMNS_B; j++) {
        printf("%d ", matrix_c[i][j]);
      }
      printf("\n");
    }
    printf("UNLOCK mutex by thread1\n");
    if ((pthread_mutex_unlock(&mut)) != 0) {
      printf("ERROR thread %d: pthread_mutex_unlock failed\n", my_id);
    }
    nloops[my_ndx]++;
    usleep(500000);
  }
}

void spi_send_thread(void *parameter) {
  int ret;
  int my_id = (int)parameter;
  int my_ndx = my_id - 1;
  int cnt;
  while (!bendoftest) {
    printf("LOCK mutex by thread2\n");
    if ((pthread_mutex_lock(&mut)) != 0) {
      printf("ERROR thread %d: pthread_mutex_lock failed\n", my_id);
    }

    uint64_t *ptr = (uint64_t *)txdata;
    for (uint8_t i = 0; i < ROWS_A; i++) {
      for (uint8_t j = 0; j < COLUMNS_B; j++) {
        *(ptr++) = matrix_c[i][j];
      }
    }
    trans.deselect = true;
    trans.delay = DELAY;
    trans.nwords = sizeof(uint64_t) * ROWS_A * COLUMNS_B;
    trans.txbuffer = txdata;
    trans.rxbuffer = rxdata;
    ret = ioctl(fd, SPIIOC_TRANSFER, seq);

    printf("MATRIX 1 %d= \n", cnt++);
    for (uint8_t i = 0; i < ROWS_A; i++) {
      for (uint8_t j = 0; j < COLUMNS_B; j++) {
        printf("0x%x ", matrix_c[i][j]);
      }
      printf("\n");
    }

    printf("UNLOCK mutex by thread2\n");
    if ((pthread_mutex_unlock(&mut)) != 0) {
      printf("ERROR thread %d: pthread_mutex_unlock failed\n", my_id);
    }
    nloops[my_ndx]++;
    usleep(500000);
  }
}

int main(int argc, FAR char *argv[]) {
  int ret;

  pthread_t thread1;
  pthread_t thread2;

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

  seq.dev = SPIDEV_ID(SPIDEVTYPE_USER, CS);
  seq.mode = CONFIG_MYSPITOOL_DEFMODE;
  seq.nbits = CONFIG_MYSPITOOL_DEFWIDTH;
  seq.frequency = CONFIG_MYSPITOOL_DEFFREQ;
  seq.ntrans = 1;
  seq.trans = &trans;

  for (int i = 0; i < COUNTER_MAX; i++) {
    txdata[0] = i;
    trans.deselect = true;
    trans.delay = DELAY;
    trans.nwords = COUNT;
    trans.txbuffer = txdata;
    trans.rxbuffer = rxdata;
    ret = ioctl(fd, SPIIOC_TRANSFER, seq);
    printf("Data %d\n", txdata[0]);
  }

  uint64_t *ptr = (uint64_t *)txdata;
  for (uint8_t i = 0; i < ROWS_A; i++) {
    for (uint8_t j = 0; j < COLUMNS_B; j++) {
      *(ptr++) = matrix_c[i][j];
    }
  }
  trans.deselect = true;
  trans.delay = DELAY;
  trans.nwords = sizeof(uint64_t) * ROWS_A * COLUMNS_B;
  trans.txbuffer = txdata;
  trans.rxbuffer = rxdata;
  ret = ioctl(fd, SPIIOC_TRANSFER, seq);

  pthread_mutex_init(&mut, NULL);

  printf("Starting thread 1\n");
  bendoftest = false;
  if ((pthread_create(&thread1, NULL, (void *)matrix_calculation_thread,
                      (void *)1)) != 0) {
    fprintf(stderr, "Error in thread#1 creation\n");
  }

  printf("Starting thread 2\n");
  if ((pthread_create(&thread2, NULL, (void *)spi_send_thread, (void *)2)) !=
      0) {
    fprintf(stderr, "Error in thread#2 creation\n");
  }

  sleep(5);

  printf("Stopping threads\n");
  bendoftest = true;
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  printf("\tThread1\tThread2\n");
  printf("Loops\t%lu\t%lu\n", nloops[0], nloops[1]);
  printf("Errors\t%lu\t%lu\n", nerrors[0], nerrors[1]);
  close(fd);

  return OK;
}

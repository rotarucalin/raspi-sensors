/* Copyright (c) Rotaru Calin
 * Distributed under the Mozilla Public License 2.0
 * Version 1.0
 */

#include <wiringPi.h>
#include <wiringSerial.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#define MAXTIMINGS	85
#define MAX_PATH 1024
const char OUTFILENAME[]="/tmp/co2sensorstate";     //can be specified via the second argument line parameter
const char LOCKFILE[]="/var/lock/co2sensorlock"; //can be specified via the third line parameter

int bStopLoop; //used to stop the loop with Ctrl-C

void sigintHandler(int sig_num)
{
  /* Reset handler to catch SIGINT next time.
 *      Refer http://en.cppreference.com/w/c/program/signal */
  signal(SIGINT, sigintHandler);
  bStopLoop = 1;
  fflush(stdout);
}

int read_mhz19_data(int fd, int *co2ppm)
{
  char cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  char cmd2[9]= {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  //error handling is missing, need to check return values read&write
  write(fd, cmd, 9);    //write 9 bytes in cmd to UART
  usleep(200000); //sleep 200ms
  read(fd, cmd2, 9);     //read 9 bytes to cmd2 from UART
  //for(int loop=0;loop<=8;loop++)
  //  printf("%x ", (int)cmd2[loop]); //hex valures for all 9 bytes read
  //printf("\n");
  int High = (int) cmd2[2];
  int Low = (int) cmd2[3];
  int ppm = (256*High)+Low;    //ppm = (256*byte2)+byte3  remember bytes numbered 0-8
  //printf("\nhigh %d\tlow %d\tppm %d\n", High, Low, ppm);
  *co2ppm = ppm;
  return 0;
}
 
long get_uptime()
{
    struct sysinfo s_info;
    int error = sysinfo(&s_info);
    if(error != 0)
    {
        printf("code error = %d\n", error);
    }
    return s_info.uptime;
}

int main(int argc, char* argv[])
{
  char strFilename[MAX_PATH];
  char strLockFilename[MAX_PATH];
  char strSerialPort[MAX_PATH];
  int  iAdditionalSleepTime = 4;
  strcpy(strFilename, OUTFILENAME);
  strcpy(strLockFilename, LOCKFILE);

  printf("Monitor MHZ19 compiled on %s at %s\n", __DATE__, __TIME__);

  strcpy(strSerialPort, "/dev/ttyAMA0");
  if(argc > 1)
    strcpy(strSerialPort, argv[1]);
  if(argc > 2)
    strcpy(strFilename, argv[2]);
  if(argc > 3)
    strcpy(strLockFilename, argv[3]);
  printf("Serial Port: %s\n", strSerialPort);
  printf("Output file: %s\n", strFilename);
  printf("Lock file: %s\n", strLockFilename);
  printf("Output cycle: %d\n", (iAdditionalSleepTime + 1));

  bStopLoop = 0;
  signal(SIGINT, sigintHandler);

  wiringPiSetup();
  int fd;  // Handler ID
  if((fd = serialOpen (strSerialPort, 9600)) < 0) // uses wiringSerial
  {
    printf("Unable to open device!\n\n");
    printf("Please check following:\n"
           "- is wiring right with rx & tx crossed? rx->tx tx->rx\n"
           "- does the device exist under /dev/\n"
           "- is the serial port activated in /boot/config.txt by enable_uart=1 (not sure about the setting)\n"
           "- is the console on the serial port in /etc/inittab disabled\n");
    return 1 ;
  }
  else
  {
    printf("serial port initialised ok\n");
  }

  long int iReadingTime = 0;

  while(!bStopLoop)
  {
    iReadingTime = get_uptime();
    int co2ppm = -1;
    char chBuffer[256];
 
    if(read_mhz19_data(fd, &co2ppm) == 0)
    {
      int lockfd = open(strLockFilename, O_WRONLY | O_CREAT);
      if(lockfd != -1)
      {
         if(flock(lockfd, LOCK_EX) == 0)
         {

           int irfd = open(strFilename, O_WRONLY | O_CREAT);
           if(irfd != -1)
           {
             sprintf(chBuffer, "CO2 Sensor Reading Time %ld\n", iReadingTime);
             write(irfd, chBuffer, strlen(chBuffer));
             sprintf(chBuffer, "PPM %d\n", co2ppm);
             write(irfd, chBuffer, strlen(chBuffer));
             close(irfd);
           }
           flock(lockfd, LOCK_UN);
         }
         close(lockfd);
      }
    }
     //sleep 800 miliseconds
    usleep(1*800*1000);
    sleep(iAdditionalSleepTime);
  }

  return 0;
}


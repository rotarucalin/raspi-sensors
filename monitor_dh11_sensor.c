/* Copyright (c) Rotaru Calin
 * Distributed under the Mozilla Public License 2.0
 * Version 1.0
*/

#include <wiringPi.h>
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

#define MAXTIMINGS	85
#define MAX_PATH 1024
#define DEFAULT_DH11_PIN 7 //the default pin where the DH11 is connected, can also be provided as parameter
const char DH11FILENAME[]="/tmp/tempsensorstate";     //can be specified via the second argument line parameter
const char LOCKFILE[]="/var/lock/tempsensorlock"; //can be specified via the third line parameter

int iOffsetT,
    iOffsetH;

int data[5] = { 0, 0, 0, 0, 0 };
int iInputPin,
    bStopLoop; //used to stop the loop with Ctrl-C

void sigintHandler(int sig_num)
{
  /* Reset handler to catch SIGINT next time.
 *      Refer http://en.cppreference.com/w/c/program/signal */
  signal(SIGINT, sigintHandler);
  bStopLoop = 1;
  fflush(stdout);
}
 
int read_dht11_dat(int *dht11_dat)
{
  uint8_t laststate	= HIGH;
  uint8_t counter		= 0;
  uint8_t j		= 0, i;
 
  dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;
 
  pinMode(iInputPin, OUTPUT);
  digitalWrite(iInputPin, LOW);
  delay(18);
  digitalWrite(iInputPin, HIGH);
  delayMicroseconds(40);
  pinMode(iInputPin, INPUT);
 
  for(i = 0; i < MAXTIMINGS; i++)
  {
    counter = 0;
    while(digitalRead(iInputPin) == laststate)
    {
      counter++;
      delayMicroseconds(1);
      if(counter == 255)
      {
        break;
      }
    }
  
   laststate = digitalRead(iInputPin);
   if(counter == 255)
     break;
 
   if ( (i >= 4) && (i % 2 == 0) )
   {
     dht11_dat[j / 8] <<= 1;
     if ( counter > 16 )
       dht11_dat[j / 8] |= 1;
     j++;
   }
  }
 
//  printf("Debug Data %d %d %d %d %d\n", dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3], dht11_dat[4]);
  if ( (j >= 40) &&
     (dht11_dat[4] == ( (dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF) ) )
  {
//    printf( "H %d.%d T %d.%d",
//            dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3]);
    return 0;
  }
  else  
  {
     return 1;
  }
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
  strcpy(strFilename, DH11FILENAME);
  strcpy(strLockFilename, LOCKFILE);
  iOffsetT = 0;
  iOffsetH = 0;

  if(argc < 2)
  {
    iInputPin = DEFAULT_DH11_PIN;
    printf("Usage monitor_dh11 <PIN> <Output File> <Lock File> <OffsetT> <OffsetH>\n");
  }
  else
    iInputPin = atoi(argv[1]);

  printf("Monitor DH11 compiled on %s at %s\n", __DATE__, __TIME__);
  if(iInputPin > 32)
    iInputPin = DEFAULT_DH11_PIN;
  printf("Working with PIN %u\n", iInputPin);

  if(argc > 2)
    strcpy(strFilename, argv[2]);
  if(argc > 3)
    strcpy(strLockFilename, argv[3]);
  if(argc > 4)
    iOffsetT = atoi(argv[4]);
  if(argc > 5)
    iOffsetH = atoi(argv[5]);
  printf("Output file: %s\n", strFilename);
  printf("Lock file: %s\n", strLockFilename);
  printf("Offsets: Temperature %d C, Humidity %d %%\n", iOffsetT, iOffsetH); 

  bStopLoop = 0;
  signal(SIGINT, sigintHandler);

  wiringPiSetup();

  long int iReadingTime = 0;
  char chBuffer[256];

  while(!bStopLoop)
  {
    iReadingTime = get_uptime();
    if(read_dht11_dat(data) == 0)
    {
      int lockfd = open(strLockFilename, O_WRONLY | O_CREAT);
      if(lockfd != -1)
      {
         if(flock(lockfd, LOCK_EX) == 0)
         {

           int irfd = open(strFilename, O_WRONLY | O_CREAT);
           if(irfd != -1)
           {
             sprintf(chBuffer, "DH11 Sensor Reading Time %ld\n", iReadingTime);
             write(irfd, chBuffer, strlen(chBuffer));
             sprintf(chBuffer, "Humidity %d.%d\n", data[0] + iOffsetH, data[1]);
             write(irfd, chBuffer, strlen(chBuffer));
             sprintf(chBuffer, "Temperature %d.%d degrees Celsius\n", data[2] + iOffsetT, data[3]);
             write(irfd, chBuffer, strlen(chBuffer));
             close(irfd);
           }
           flock(lockfd, LOCK_UN);
         }
         close(lockfd);
      }

    }
    //sleep 1 second
    usleep(1*1000*1000);

  }

  return 0;
}

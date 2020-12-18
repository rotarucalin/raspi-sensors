/* Copyright (c) Rotaru Calin
 * Distributed under the Mozilla Public License 2.0
 * Version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h> 
#include <wiringPi.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <fcntl.h>

int iInputPin;
int bStopLoop;
#define MAXHISTORY 4
#define MAX_PATH 1024
int aiHistoricalValues[MAXHISTORY];
const char IRFILENAME[]="/tmp/sensorstate";     //can be specified via the second argument line parameter
const char LOCKFILE[]="/var/lock/irsensorlock"; //can be specified via the third line parameter

void sigintHandler(int sig_num) 
{ 
  /* Reset handler to catch SIGINT next time. 
     Refer http://en.cppreference.com/w/c/program/signal */
  signal(SIGINT, sigintHandler); 
  bStopLoop = 1;
  fflush(stdout); 
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
  char strIRFilename[MAX_PATH];
  char strLockFilename[MAX_PATH];
  strcpy(strIRFilename, IRFILENAME);
  strcpy(strLockFilename, LOCKFILE);  
  
  if(argc < 2)
    iInputPin = 2;
  else
    iInputPin = atoi(argv[1]);
 
  printf("Monitor IR compiled on %s at %s\n", __DATE__, __TIME__); 
  if(iInputPin > 32)
    iInputPin = 2;
  printf("Working with PIN %u\n", iInputPin);
  
  if(argc > 2)
    strcpy(strIRFilename, argv[2]);
  if(argc > 3)
    strcpy(strLockFilename, argv[3]);
  printf("Output file: %s\n", strIRFilename);
  printf("Lock file: %s\n", strLockFilename);

  bStopLoop = 0;
  signal(SIGINT, sigintHandler);

  wiringPiSetup();
  pinMode(iInputPin, INPUT);

  long int iONTime = 0;
  long int iOFFTime = 0;
  int iLastValue = 0;
 
  while(!bStopLoop)
  {
    int iValue = digitalRead(iInputPin);
    if(iValue != iLastValue)
    {
      //we use a historical record to eliminate single flickering
      //fill in history
      for(int iCounter = MAXHISTORY - 1; iCounter > 0; iCounter--)
        aiHistoricalValues[iCounter] = aiHistoricalValues[iCounter - 1];
      aiHistoricalValues[0] = iValue;
      //check if history is consistent
      int iHistoryConsistent = 1;
      for(int iCounter = MAXHISTORY - 1; iCounter > 0; iCounter--)
        if(aiHistoricalValues[iCounter] != aiHistoricalValues[iCounter - 1])
        {
          iHistoryConsistent = 0;
          break;
        }
      if(!iHistoryConsistent)
      {
        if(iValue)
          usleep(200 * 1000);
        else
          usleep(100 * 1000);
        continue;
      }

      iLastValue = iValue;
      if(iValue)
        iONTime = get_uptime();
      else
        iOFFTime = get_uptime();

      char chBuffer[256];
      //wait to aquire lock
      int lockfd = open(strLockFilename, O_WRONLY | O_CREAT);
      if(lockfd != -1)
      {
         if(flock(lockfd, LOCK_EX) == 0)
         {

           int irfd = open(strIRFilename, O_WRONLY | O_CREAT);
           if(irfd != -1)
           {
             sprintf(chBuffer, "state %d\n", iValue);
             write(irfd, chBuffer, strlen(chBuffer));
             sprintf(chBuffer, "timestamp_ms_ON %ld\n", iONTime);
             write(irfd, chBuffer, strlen(chBuffer));
             sprintf(chBuffer, "timestamp_ms_OFF %ld\n", iOFFTime);
             write(irfd, chBuffer, strlen(chBuffer));
             close(irfd); 
           }
           flock(lockfd, LOCK_UN); 
         }
         close(lockfd);
      }
    }

    //sleep 200ms
    usleep(200 * 1000);
  }

  return 0;
}

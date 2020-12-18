/* Copyright (c) Rotaru Calin
 * Distributed under the Mozilla Public License 2.0
 * Version 1.0
 */

#include <wiringPi.h>
#include <softPwm.h>
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

#define MAXPWM 1023
#define MAX_PATH 1024
#define DEFAULT_PWM_PIN 1 //the default pin where the PWM output will occur, can also be provided as parameter

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
  if(argc < 2)
  {
    iInputPin = DEFAULT_PWM_PIN;
    printf("pwm_transition <PIN> <Start> <Stop> <Total Duration ms>\n");
    printf("PWM Range is 0..1023, we execute steps at least 25 ms long");
  }
  else
    iInputPin = atoi(argv[1]);

  printf("PWM Transition compiled on %s at %s\n", __DATE__, __TIME__);
  if(iInputPin > 32)
    iInputPin = DEFAULT_PWM_PIN;
  printf("Working with PIN %u\n", iInputPin);

  int iStart = 0, iStop = 1023, iDuration = 1000;

  wiringPiSetup();
  if(argc > 2)
    iStart = atoi(argv[2]);
  if(argc > 3)
    iStop = atoi(argv[3]);
  if(argc > 4)
    iDuration = atoi(argv[4]);
  int iDirection;
  if(iStart > iStop)
    iDirection = -1;
  else
    iDirection = +1;
  if((iStart > 1023) || (iStart < 0) || (iStop > 1023) || (iStop < 0))
  {
    printf("Parameter out of range 0..1023\n");
    return 1;
  }
  softPwmCreate(iInputPin, iStart, 1023);
  if(iStart == iStop)
    return 0; //nothing to do 
  
  int iStepDuration = iDuration / (iStop - iStart) * iDirection;
  if(iStepDuration < 25)
  {
    iDirection = iDirection * (25 / (iStepDuration + 1) + 1);
    iStepDuration = iDuration / (iStop - iStart) * iDirection;
  }
  printf("PWM from: %d to %d in %d ms, step duration %d, step size %d\n", iStart, iStop, iDuration, iStepDuration, iDirection);
  bStopLoop = 0;
  signal(SIGINT, sigintHandler);

  for(int iValue = iStart; ((iStop - iValue) * iDirection > 0) && (bStopLoop == 0); iValue += iDirection)
  {
    softPwmWrite(iInputPin, iValue);
    usleep(iStepDuration * 1000);
  }

  return 0;
}

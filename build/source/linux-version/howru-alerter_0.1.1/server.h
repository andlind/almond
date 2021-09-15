#ifndef HOWRU_ALERT_SERVER_H
#define HOWRU_ALERT_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Server {
  char name[100];
  int port;
  int active;
  char group[50];
};

#endif

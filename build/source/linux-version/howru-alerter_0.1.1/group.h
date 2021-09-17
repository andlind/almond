#ifndef HOWRU_ALERT_GROUP_H
#define HOWRU_ALERT_GROUP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Group {
  char name[50];
  int active;
  int escalation;
  char email[500];
};

#endif

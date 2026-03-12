#include <limits.h>

#ifndef ELEVATE_H
#define ELEVATE_H

int fw(int a, int b, int *dests, int numPeople); // used in multiple functions (implemented in recurse.c)

typedef struct elevator_data { // input data for calculation
  int numFloors;
  int *dests;
  int numPeople;
  int numStops;
} elevData;

typedef struct calculation_data { // output data of calculation
  int lastStop;
  int *minStops;
  int minNumStops;
  int minCost;
} calcData;

void recurse(elevData data, calcData *res);

int memoize(elevData data, calcData *res);

int brute(elevData data, calcData *res);

int dp(elevData data, calcData *res, char debug);

#endif
#include "elevate.h"
#include <stdio.h>
#include <stdlib.h>

int fw(int a, int b, int *dests, int numPeople) // finds the sum of the number of floors that passangers with destinations between a and b would have to walk
{
  int sum = 0;                          // total floors walked
  for (int i = 0; i < numPeople; i++) { // loop through all destinations
    if (dests[i] <= a || dests[i] > b)
      continue;
    if (b == INT_MAX) { // int max is considered infinity
      sum += dests[i] - a;
    }
    else { // count the smallest of the 2 distances
      int floorsToA = dests[i] - a;
      int floorsToB = b - dests[i];
      sum += (floorsToA < floorsToB) ? floorsToA : floorsToB;
    }
  }
  return sum;
}

int M(int i, int j, int *dests, int numPeople) // recursive function based on formula explain on README
{
  if (i == 0)
    return fw(0, INT_MAX, dests, numPeople);
  int min = M(i - 1, 0, dests, numPeople) - fw(0, INT_MAX, dests, numPeople) + fw(0, j, dests, numPeople) + fw(j, INT_MAX, dests, numPeople);
  for (int k = 1; k <= j; k++) { // looping through every past stop to find the most optimal combination(with minimum cost for j as the last stop)
    int cost = M(i - 1, k, dests, numPeople) - fw(k, INT_MAX, dests, numPeople) + fw(k, j, dests, numPeople) + fw(j, INT_MAX, dests, numPeople);
    if (cost < min) {
      min = cost;
    }
  }
  return min;
}

void recurse(elevData data, calcData *res)
{
  int minCost;
  int lastStop = 0;
  if (data.numStops == 0) { // no stops
    minCost = fw(0, INT_MAX, data.dests, data.numPeople);
  }
  else if (data.numStops >= data.numPeople) { // if the elevator can make a stop at every destination there is no cost
    lastStop = data.numFloors;
    minCost = 0;
  }
  else {
    minCost = M(data.numStops, 0, data.dests, data.numPeople);    // initializing with the cost of making the last stop at floor 0 (j=0)
    for (int j = 1; j <= data.numFloors; j++) {                   // looping through each floor available as the last stop
      int cost = M(data.numStops, j, data.dests, data.numPeople); // calulating cost
      if (cost < minCost) {                                       // finding the minimum cost
        minCost = cost;
        lastStop = j;
      }
    }
  }
  (*res).lastStop = lastStop;
  (*res).minCost = minCost;
}
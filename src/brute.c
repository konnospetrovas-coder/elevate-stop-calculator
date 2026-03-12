#include "elevate.h"
#include <stdio.h>
#include <stdlib.h>

int calcWalks(int n, int *stops, int *dests, int numPeople) // calculates total cost given an array of stops
{
  int sum = fw(0, stops[0], dests, numPeople); // initializing sum with fw from 0 to the first stop
  int i;
  for (i = 1; i <= n; i++) {
    sum += fw(stops[i - 1], stops[i], dests, numPeople); // adding fw from current stop to the next stop
  }
  sum += fw(stops[i - 1], INT_MAX, dests, numPeople); // adding fw from last stop to infinity
  return sum;
}

int brute(elevData data, calcData *res)
{
  int minCost = fw(0, INT_MAX, data.dests, data.numPeople); // initializing with the cost of stoping only at floor 0
  if (data.numStops == 0) {                                 // no stops
    (*res).lastStop = 0;
    (*res).minCost = minCost;
    return 0;
  }
  else {
    int *stops = calloc(data.numStops, sizeof(int)); // array with the cunnent stop configuration
    if (stops == NULL) {
      fprintf(stderr, "Failed to allocate stops array\n");
      return 1;
    }
    int *minStops = calloc(data.numStops, sizeof(int)); // array with the minimum stop configuration
    if (minStops == NULL) {
      fprintf(stderr, "Failed to allocate minimum stops array\n");
      free(stops);
      return 1;
    }
    int i;
    for (i = 0; i < data.numStops && i < data.numFloors; i++) { // looping for every possible number of stops to check if its possible with less than the given amount
      // i should be smaller than numFloors because every stop is unique in this implementation and exceeding numFloors whould not result in minimum cost
      for (int j = 0; j <= i; j++) { // generate starting configuration for current number of stops, every stop must be smaller than its next one
        stops[j] = j + 1;
      }
      while (1) {
        int cost = calcWalks(i, stops, data.dests, data.numPeople); // calculate the full cost for the current configuration
        if (cost < minCost) {                                       // check if its smaller
          for (int j = 0; j <= i; j++) {                            // and copy into minStops
            minStops[j] = stops[j];
          }
          minCost = cost;
        }
        int j = i;
        while (j >= 0 && stops[j] == data.numFloors + j - i) // find the last element that does not have its max value (data.numFloors + j - i , because every stop must be smaller than its next one)
          j--;
        if (j < 0) // stop if every number has its max value
          break;
        stops[j]++; // increament the stop
        char invalid = 0;
        for (int k = j + 1; k <= i; k++) { // change all the elements to the right side (with an invalid check)
          stops[k] = stops[k - 1] + 1;
          if (stops[k] > data.numFloors) { // invalid if one element exceeds the number of floors
            invalid = 1;
            break;
          }
        }

        if (invalid) // break when invalid
          break;
      }
    }
    int j = 0;
    while (j < i && minStops[j] > 0) j++; // get effective number of stops, if the minimum cost was found without using all available numStops
    (*res).minStops = minStops;
    (*res).minCost = minCost;
    (*res).minNumStops = j;
    (*res).lastStop = minStops[j - 1];
    free(stops);
    return 0;
  }
}
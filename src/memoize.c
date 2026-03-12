#include "elevate.h"
#include <stdio.h>
#include <stdlib.h>

/*
recursive function based on given formula
but if the element that is to be calculated is already stored just get the value from the array
*/
int M_mem(int i, int j, int *dests, int numPeople, int **mem_arr)
{
  if (mem_arr[i][j] != -1) // if it is already stored in the array don't calculate it again
    return mem_arr[i][j];
  if (i == 0) { // if i = 0 store and return it
    mem_arr[i][j] = fw(0, INT_MAX, dests, numPeople);
    return mem_arr[i][j];
  } // calculate with recursive formula
  int min = M_mem(i - 1, 0, dests, numPeople, mem_arr) - fw(0, INT_MAX, dests, numPeople) + fw(0, j, dests, numPeople) + fw(j, INT_MAX, dests, numPeople);
  for (int k = 1; k <= j; k++) { // looping through every past stop to find the most optimal combination(with minimum cost for j as the last stop)
    int cost = M_mem(i - 1, k, dests, numPeople, mem_arr) - fw(k, INT_MAX, dests, numPeople) + fw(k, j, dests, numPeople) + fw(j, INT_MAX, dests, numPeople);
    if (cost < min) {
      min = cost;
    }
  }
  mem_arr[i][j] = min; // store it into the array
  return min;
}

int memoize(elevData data, calcData *res)
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
    int **mem_arr = malloc((data.numStops + 1) * sizeof(int *)); // creating 2d array
    if (mem_arr == NULL) {
      fprintf(stderr, "Failed to allocate rows\n");
      return 1;
    }
    for (int i = 0; i <= data.numStops; i++) {
      mem_arr[i] = malloc((data.numFloors + 1) * sizeof(int)); // allocating comlumns
      if (mem_arr[i] == NULL) {
        for (int j = 0; j < i; j++) free(mem_arr[j]);
        fprintf(stderr, "Failed to allocate column %d\n", i);
        free(mem_arr);
        return 1;
      }
      for (int j = 0; j <= data.numFloors; j++) { // and initializing all values with -1
        mem_arr[i][j] = -1;
      }
    }
    minCost = M_mem(data.numStops, 0, data.dests, data.numPeople, mem_arr);    // same logic as recurse, initializing with j=0
    for (int j = 1; j <= data.numFloors; j++) {                                // looping through each floor available as the last stop
      int cost = M_mem(data.numStops, j, data.dests, data.numPeople, mem_arr); // calulating cost
      if (cost < minCost) {                                                    // finding the minimum cost
        minCost = cost;
        lastStop = j;
      }
    }
    for (int i = 0; i <= data.numStops; i++) // freeing 2d array
      free(mem_arr[i]);
    free(mem_arr);
  }
  (*res).lastStop = lastStop;
  (*res).minCost = minCost;
  return 0;
}
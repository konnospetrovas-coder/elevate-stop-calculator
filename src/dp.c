#include "elevate.h"
#include <stdio.h>
#include <stdlib.h>

void swap(int *a, int *b)
{
  int temp = *a;
  *a = *b;
  *b = temp;
}

int dp(elevData data, calcData *res, char debug)
{
  int minCost;
  if (data.numStops == 0) { // no stops
    (*res).minCost = fw(0, INT_MAX, data.dests, data.numPeople);
    (*res).lastStop = 0;
    if (debug) { // just print debug
      for (int j = 0; j <= data.numFloors; j++) {
        printf("%d\t", (*res).minCost);
      }
      printf("\n");
    }
    return 0;
  }
  else {
    int **M_arr = malloc(2 * sizeof(int *)); // array that stores the M of the formula in only 2 rows (only storing the last 2 rows is needed)
    if (M_arr == NULL) {
      fprintf(stderr, "Failed to allocate memory array\n"); // alocate rows
      return 1;
    }
    for (int i = 0; i < 2; i++) {
      M_arr[i] = malloc((data.numFloors + 1) * sizeof(int)); // alocate columns
      if (M_arr[i] == NULL) {
        for (int j = 0; j < i; j++) free(M_arr[j]);
        fprintf(stderr, "Failed to allocate memory array\n");
        free(M_arr);
        return 1;
      }
    }
    int **Min_K = malloc(data.numStops * (data.numFloors + 1) * sizeof(int *)); // array that stores the minimum K that was used to minimise the cost for last stop
    if (Min_K == NULL) {
      fprintf(stderr, "Failed to allocate minimum K array\n"); // alocate rows
      for (int i = 0; i < 2; i++)                              // freeing memory
        free(M_arr[i]);
      free(M_arr);
      return 1;
    }
    for (int i = 0; i < data.numStops; i++) {
      Min_K[i] = malloc((data.numFloors + 1) * sizeof(int)); // alocate columns
      if (Min_K[i] == NULL) {
        for (int j = 0; j < i; j++) free(M_arr[j]);
        fprintf(stderr, "Failed to allocate minimum K array\n");
        free(Min_K);
        for (int i = 0; i < 2; i++) // freeing memory
          free(M_arr[i]);
        free(M_arr);
        return 1;
      }
    }
    minCost = fw(0, INT_MAX, data.dests, data.numPeople); // initialize min cost
    for (int j = 0; j <= data.numFloors; j++) {
      M_arr[0][j] = minCost; // initialize first row
      if (debug)
        printf("%d\t", M_arr[0][j]);
    }
    if (debug)
      putchar('\n');
    int lastStop = 0;

    int i;
    for (i = 1; i <= data.numStops; i++) {        // using recursive formula iteratively
      for (int j = 0; j <= data.numFloors; j++) { // looping through each possible last stop for i stops in total
        int min = M_arr[!(i & 1)][0] - fw(0, INT_MAX, data.dests, data.numPeople) + fw(0, j, data.dests, data.numPeople) + fw(j, INT_MAX, data.dests, data.numPeople);
        //!(i & 1) and i & 1 are used to address the 2 rows so that every loop the reading and the writting row swap
        Min_K[i - 1][j] = 0;
        for (int k = 1; k <= j; k++) { // looping through all of the stops before j to find the one that results in the minimum cost from the other row of the memory array
          int cost = M_arr[!(i & 1)][k] - fw(k, INT_MAX, data.dests, data.numPeople) + fw(k, j, data.dests, data.numPeople) + fw(j, INT_MAX, data.dests, data.numPeople);
          if (cost < min) {
            min = cost;          // finding the minimum based on the row of the saved M for the previous i
            Min_K[i - 1][j] = k; // storing the previous stop that was used to achieve that minimum in order to calculate the optimal stop path at the end
          }
        }
        M_arr[i & 1][j] = min; // storing into the other row of the memory array
        if (min < minCost) {   // finding total minimum and last stop
          minCost = min;
          lastStop = j;
        }
      }
      if (debug) { // print row for debug
        for (int j = 0; j <= data.numFloors; j++) {
          printf("%d\t", M_arr[i & 1][j]);
        }
        putchar('\n');
      }
    }
    int *minStops = calloc(data.numStops, sizeof(int)); // the optimal stops array
    if (minStops == NULL) {
      fprintf(stderr, "Failed to allocate minimum stops array\n");
      for (int i = 0; i < 2; i++)
        free(M_arr[i]);
      free(M_arr);
      for (int i = 0; i < data.numStops; i++)
        free(Min_K[i]);
      free(Min_K);
      return 1;
    }
    (*res).lastStop = lastStop;
    int n = 0;
    while (lastStop != 0) { // constructing the optimal stops array by traversing the 2d array based on the last stop and the stops that resulted in the minimum cost (on the first row all are 0)
      minStops[n] = lastStop;
      lastStop = Min_K[i - 2 - n][lastStop];
      n++;
    }
    for (int i = 0; i < n / 2; i++) {           // swapping the order of the array to ascending
      swap(minStops + i, minStops + n - i - 1); // swapping pointers
    }
    (*res).minNumStops = n; // pasing data through res as an interface
    (*res).minStops = minStops;
    (*res).minCost = minCost;

    for (int i = 0; i < 2; i++) // freeing memory
      free(M_arr[i]);
    free(M_arr);
    for (int i = 0; i < data.numStops; i++)
      free(Min_K[i]);
    free(Min_K);
  }
  return 0;
}

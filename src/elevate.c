#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elevate.h"

void handleRecurse(elevData data, calcData res, int numStops)
{
  recurse(data, &res);
  if (numStops == 0) printf("No lift stops\n");
  else printf("Last stop at floor: %d\n", res.lastStop);
  printf("The minimum cost is: %d\n", res.minCost);
}

int handleBrute(elevData data, calcData res, int numStops)
{
  if (brute(data, &res) == 1) return 1; // allocation error
  if (numStops == 0) printf("No lift stops\n");
  else {
    printf("Lift stops are:"); // print stops
    for (int i = 0; i < res.minNumStops; i++) {
      printf(" %d", res.minStops[i]);
    }
    putchar('\n');
    free(res.minStops);
  }
  printf("The minimum cost is: %u\n", res.minCost);
  return 0;
}

int handleMemoize(elevData data, calcData res, int numStops)
{
  if (memoize(data, &res) == 1) return 1; // allocation error
  if (numStops == 0) printf("No lift stops\n");
  else printf("Last stop at floor: %d\n", res.lastStop);
  printf("The minimum cost is: %d\n", res.minCost);
  return 0;
}

int handleDp(elevData data, calcData res, int numStops, int debug)
{
  if (dp(data, &res, debug) == 1) return 1; // allocation error
  if (numStops == 0) printf("No lift stops\n");
  else {
    printf("Lift stops are:"); // print stops
    for (int i = 0; i < res.minNumStops; i++) {
      printf(" %d", res.minStops[i]);
    }
    putchar('\n');
    free(res.minStops);
  }
  printf("The minimum cost is: %u\n", res.minCost);
  return 0;
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "Usage: ./elevate <filepath>\n--mode argument to specify the algorith of the search.\n");
    return 1;
  }
  int numPeople;
  int numStops;
  FILE *file = fopen(argv[1], "r"); // reading file
  if (file == NULL) {
    fprintf(stderr, "File %s could not be read.\n", argv[1]);
    return 1;
  }
  // getting data
  if (fscanf(file, "%d %d", &numPeople, &numStops) != 2 || numPeople == 0) return 1;
  if (getc(file) != '\n') return 1;
  int *dests = malloc(numPeople * sizeof(int));
  int numFloors = 0;
  for (int i = 0; i < numPeople; i++) {
    if (fscanf(file, "%d", &dests[i]) != 1) {
      free(dests);
      return 1;
    }
    if (dests[i] > numFloors) { // getting max for numFloors
      numFloors = dests[i];
    }
  }
  fclose(file);
  elevData data = {numFloors, dests, numPeople, numStops}; // input data
  calcData res;                                            // output data
  if (argc > 2) {
    if (argc > 4) {
      fprintf(stderr, "Invalid argument count\n");
      free(dests);
      return 1;
    }
    if (strcmp(argv[2], "--mode=dp") == 0) {
      if (argc == 4) {
        if (strcmp(argv[3], "--debug") == 0) {
          if (handleDp(data, res, numStops, 1) == 1) {
            free(dests);
            return 1;
          }
        }
        else {
          fprintf(stderr, "Invalid argument %s\n", argv[2]);
          free(dests);
          return 1;
        }
      }
      else {
        if (handleDp(data, res, numStops, 0) == 1) {
          free(dests);
          return 1;
        }
      }
    }
    else if (argc > 3) { // only dp mode can have more than 3 arguments
      fprintf(stderr, "Invalid argument count\n");
      free(dests);
      return 1;
    }
    else if (strcmp(argv[2], "--mode=recurse") == 0) {
      handleRecurse(data, res, numStops);
    }
    else if (strcmp(argv[2], "--mode=brute") == 0) {
      if (handleBrute(data, res, numStops) == 1) {
        free(dests);
        return 1;
      }
    }
    else if (strcmp(argv[2], "--mode=memoize") == 0) {
      if (handleMemoize(data, res, numStops) == 1) {
        free(dests);
        return 1;
      }
    }
    else {
      fprintf(stderr, "Invalid argument %s\n", argv[2]);
      free(dests);
      return 1;
    }
  }
  else { // default dp with no debug
    if (handleDp(data, res, numStops, 0) == 1) {
      free(dests);
      return 1;
    }
  }
  free(dests);
  return 0;
}
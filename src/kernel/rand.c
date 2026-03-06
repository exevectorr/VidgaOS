#include "rand.h"

static unsigned int rand_state = 1;

void srand(unsigned int seed) { rand_state = seed ? seed : 1; }

int rand(void) {
  rand_state = rand_state * 1103515245 + 12345;
  return (int)((rand_state >> 16) & 0x7FFF);
}

int rand_range(int min, int max) {
  if (min >= max)
    return min;
  return min + (rand() % (max - min + 1));
}

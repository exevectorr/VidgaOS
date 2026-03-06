#ifndef RAND_H
#define RAND_H

/* Seed the RNG (call once, e.g. from pit_ticks) */
void srand(unsigned int seed);

/* Return a pseudo-random number 0..32767 */
int rand(void);

/* Return a pseudo-random number in [min, max] inclusive */
int rand_range(int min, int max);

#endif

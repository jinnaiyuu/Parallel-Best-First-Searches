#include <stdio.h>
#include <cstdlib>
#include <time.h>

/**
 *
 * input <height> <width> <prob of wall>
 * 
 * ex. ./grid_gen 10-10-0.5
 */


int main(int argc, char* argv[]) {
  srand(time(NULL));
  unsigned int number_of_cities;
  FILE* outfile;
  char filename[30];

  if(argc > 1 && sscanf(argv[1], "%u", &number_of_cities) == 1) {
    int number = 1;
    if (argc > 2 && sscanf(argv[2], "%d", &number) != 1) {
      number = 1;
    }
    
    for (int num = 0; num < number; ++num) {
      sprintf(filename, "%d_%d", number_of_cities, num);
      outfile = fopen(filename, "w");

      fprintf(outfile,"%d\n", number_of_cities);

      for (int i = 0; i < number_of_cities; ++i) {
	double x, y;
	x = ((double) rand() / (RAND_MAX));
	y = ((double) rand() / (RAND_MAX));
	fprintf(outfile,"%lf %lf\n", x, y);
      }

      fclose(outfile);
    }
  } else {
    printf("input format error\n should be two integers like ./tsp_gen 10 5 \n");
  }
}

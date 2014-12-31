#include <stdio.h>
#include <cstdlib>
#include <time.h>

/**
 *
 * input <height> <width> <prob of wall>
 * 
 * ex. ./grid_gen 10-10-0.5
 */

#define WIDTH 5000
#define HEIGHT 5000

static bool board[WIDTH][HEIGHT];

int main(int argc, char* argv[]) {
  srand(time(NULL));
  double prob;
  FILE* outfile;
  char filename[30];

  if(argc > 1 && sscanf(argv[1], "%lf", &prob) == 1) {
    int p = static_cast<int>(prob * RAND_MAX);
    int number = 1;
    if (argc > 2 && sscanf(argv[2], "%d", &number) != 1) {
      number = 1;
    }
    
    for (int num = 0; num < number; ++num) {
      sprintf(filename, "%d_%d-%d-%s", num, WIDTH, HEIGHT, argv[1]);
      outfile = fopen(filename, "w");

      for (int x = 0; x < WIDTH; ++x) {
	for (int y = 0; y < HEIGHT; ++y) {
	  if (rand() < p) {
	    board[x][y] = false;
	  } else {
	    board[x][y] = true;
	  }
	}
      }
    
      int startx, starty, goalx, goaly;
      do {
	startx = (rand() % WIDTH) / 3;
	starty = (rand() % HEIGHT) / 3;
      } while (board[startx][starty]);

      do {
	goalx = (rand() % WIDTH) / 3 + WIDTH * 2 / 3;
	goaly = (rand() % HEIGHT) / 3 + HEIGHT * 2 / 3;
      } while (board[goalx][goaly]);

      fprintf(outfile,"%d %d\n", HEIGHT, WIDTH);
      fprintf(outfile,"Board:\n");
      for (int y = 0; y < HEIGHT; ++y) {
	for (int x = 0; x < WIDTH; ++x) {
	  if (board[x][y]) {
	    fprintf(outfile,"#");
	  } else {
	    fprintf(outfile," ");
	  }
	}
	fprintf(outfile,"\n");
      }
      fprintf(outfile,"Unit\n");
      fprintf(outfile,"Four-way\n");
      fprintf(outfile,"%d %d %d %d\n", startx, starty, goalx, goaly);
      fclose(outfile);
    }
  } else {
    printf("input format error\n should be like 10-10-0.5\n");
  }
}

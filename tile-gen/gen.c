/* © 2014 the PBNF Authors under the MIT license. See AUTHORS for the list of authors.*/

#include <stdlib.h>
#include <stdio.h>

int size = 0;

int counter = 0;

void permuteRecursive(int *dir, int *flag, int index)
{
	int i, j;
	int ground = 1;
	int revOrd = 0;

	for (i = 0; i < size; i++) {
		if(flag[i] == 0) {
			ground = 0;
			flag[i] = 1;
			dir[i] = index;
			permuteRecursive(dir, flag, index + 1);
			flag[i] = 0;
			dir[i] = 0;
		}
	}
	if (ground) {
		for (i = 0; i < size - 1; i++) 
			for (j = i + 1; j < size; j++) 
				if (dir[i] > dir[j] && dir[j] != 0)
					revOrd++;
		if (revOrd % 2 == 0) {
			printf("%d", ++counter);    
			for (i = 0; i < size; i++)
				printf(" %d", dir[i]); 
			printf("\n");
			printf("%i\n", revOrd);
		}
	}
}

void permute()
{
	int *dir = (int *) calloc (size, sizeof(int));
	int *flag = (int *) calloc (size, sizeof(int));
	permuteRecursive(dir, flag, 0);
	free(dir);
	free(flag);
}

main (int argc, char *argv[])
{
	size = atoi(argv[1]);
	permute();
}


#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


struct random_walk_result
{
  bool dest;
  int walk_count;
}

random_walk_result walk(int a, int b, int x, double p, unsigned int seed) 
{

  random_walk_result res;
  res.dest = 0;
  res.walk_count = 0;
  while (x != a && x != b) 
  {
    if ((double) rand_r(&seed) <= (double) RAND_MAX * p)
      x += 1;
    else
      x -= 1;
    res.walk_count++;
  }
  if(x == a)
  {
	  res.dest = 0;
  }
  else
  {
	  res.dest = 1;
  }
  return res;
}

int main(int argc, char **argv) 
{

  int a, b, x, N, P;
  double p;
  if(argc >= 7)
  {
	  printf("Incorrect number of arguments.\n");
	  exit(1);
  }
  a = atoi(argv[1]);
  b = atoi(argv[2]);
  x = atoi(argv[3]);
  N = atoi(argv[4]);
  p = atof(argv[5]);
  P = atoi(argv[6]);
  int reached_b = 0;   
  int total_moves = 0;  

  omp_set_num_threads(P);

  struct timespec begin, end;
  clock_gettime(CLOCK_REALTIME,&begin);


  #pragma omp parallel for reduction(+:total_moves,reached_b)
  for (size_t i = 0; i < N; i++) 
  {
	unsigned seed = clock();
    random_walk_result res = walk(a, b, x, p, seed);
    if(res.dest == 1)
    {
		reached_b += 1;
	}
    total_moves += res.walk_count;
  }

  clock_gettime(CLOCK_REALTIME,&end);

  double b_probability = ((double) reached_b) / ((double)N);
  double average_lifetime = (double) total_moves / ((double)N);
  double execution_time = (end.tv_sec - begin.tv_sec) + (end.tv_nsec - begin.tv_nsec)/1000000000.0;

  FILE *file;
  file = fopen("stats.txt", "a");
  if (file == NULL) {
    printf("Could not create 'stats.txt'.\n");
    exit(1);
  }
  fprintf(file, "%f %f %fs %d %d %d %d %f %d\n", b_probability, average_lifetime, execution_time, a, b, x, N, p, P);
  fclose(file);
  return 0;
}

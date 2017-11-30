#include <assert.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

int sort_compare(const void* a, const void* b) {
  int a_val = *((const int*)a);
  int b_val = *((const int*)b);
  return a_val - b_val;
}

void print_buf(int* arr, size_t count, FILE* file) {
  size_t i;
  for (i = 0; i < count; i++) {
    fprintf(file, "%d ", arr[i]);
  }
  fprintf(file, "\n");
}


size_t upper_bound(int* array, size_t size, int element) {
  if (array[0] > element)
    return 0;
  size_t a = 0;
  size_t b = size;
  while (b - a > 1) {
    size_t c = (a + b) / 2;
    if (array[c] > element) {
      b = c;
    } 
    else {
      a = c;
    }
  }
  return b;
}


void merge(int* first, int* second, int* buffer, size_t first_arr_count, size_t second_arr_count) {
  size_t length = first_arr_count + second_arr_count;
  size_t first_arr_cursor = 0;
  size_t second_arr_cursor = 0;
  size_t buffer_cursor = 0;
  if (first_arr_count == 0) {
    memcpy(buffer, second, second_arr_count * sizeof(int));
    return;
  } else if (second_arr_count == 0) {
    memcpy(buffer, first, first_arr_count * sizeof(int));
    return;
  }
  while (buffer_cursor < length) {
    if (first_arr_cursor == first_arr_count || (second_arr_cursor < second_arr_count && second[second_arr_cursor] < first[first_arr_cursor])) {
      buffer[buffer_cursor] = second[second_arr_cursor];
      ++second_arr_cursor;
    } 
    else if (second_arr_cursor == length || second[second_arr_cursor] >= first[first_arr_cursor]) {
      buffer[buffer_cursor] = first[first_arr_cursor];
      ++first_arr_cursor;
    }
    buffer_cursor++;
  }
}


void merge_parallel(int* first, int* buffer, size_t first_arr_count, size_t second_arr_count) {
  int* big;
  int* small;
  size_t big_count;
  size_t small_count;
  if (first_arr_count < second_arr_count) {
    small = first;
    small_count = first_arr_count;
    big = first + first_arr_count;
    big_count = second_arr_count;
  } 
  else {
    small = first + first_arr_count;
    small_count = second_arr_count;
    big = first;
    big_count = first_arr_count;
  }
  size_t big_half = first_arr_count / 2;
  int big_median = big[big_half];
  size_t small_upper = upper_bound(small, small_count, big_median);
  int* right_buffer = buffer + big_half + small_upper;

  if (big_count == first_arr_count) {
    #pragma omp task
    merge(big, small, buffer, big_half, small_upper);
    #pragma omp task
    merge(big + big_half, small + small_upper, right_buffer, big_count - big_half, small_count - small_upper);
  }
  else {
    #pragma omp task
    merge(small, big, buffer, small_upper, big_half);
    #pragma omp task
    merge(small + small_upper, big + big_half, right_buffer,
          small_count - small_upper, big_count - big_half);
  }
  #pragma omp taskwait
  memcpy(first, buffer, (second_arr_count + first_arr_count) * sizeof(int));
}


void recurs(int* arr, int* buffer, size_t count, size_t chunk_size) {
  if (count <= chunk_size) {
    qsort(arr, count, sizeof(int), sort_compare);
  } 
  else {
    size_t middle = count / 2;
#pragma omp task
    recurs(arr, buffer, middle, chunk_size);
#pragma omp task
    recurs(arr + middle, buffer + middle, count - middle, chunk_size);
#pragma omp taskwait
    merge_parallel(arr, buffer, middle, count - middle);
  }
}


void merge_sort(int* arr, size_t length, size_t chunk_size, int threads) {
  int* buffer = malloc(sizeof(int) * length);

#pragma omp parallel num_threads(threads)
  {
#pragma omp single
    recurs(arr, buffer, length, chunk_size);
  }
  free(buffer);
}

int main(int argc, char* argv[]) {
  int* merge_array;
  int* qsort_array;
  size_t length;
  size_t chunk_size;
  int threads;
  FILE* stats = fopen("stats.txt", "w");
  FILE* qsort_stats = fopen("qsort_stats.txt", "w");
  FILE* data = fopen("data.txt", "w");
  
  if(stats == NULL || qsort_stats == NULL || data == NULL) {
	  printf("Could not create file.\n");
      exit(1);
  }
  srand(time(NULL));
  if(argc >= 4)
  {
	  printf("Incorrect number of arguments.\n");
	  exit(1);
  }
  length = atoi(argv[1]);
  chunk_size = atoi(argv[2]);
  threads = atoi(argv[3]);
  
  merge_array = malloc(sizeof(int) * length);
  qsort_array = malloc(sizeof(int) * length);
  
  size_t i;
  for (i = 0; i < count; i++) {
    merge_array[i] = rand() % 16;
  )
  memcpy(qsort_array, merge_array, sizeof(int) * length);
  print_buf(merge_array, length, data);
  
  double begin = omp_get_wtime();
  merge_sort(merge_array, length, chunk_size, threads);
  double time_spent_merge = omp_get_wtime() - begin;
  print_buf(merge_array, length, data);
  fclose(data);
  fprintf(stats, "%fs %d %d %d\n", time_spent_merge, length, chunk_size, threads);
  fclose(stats);


  begin = omp_get_wtime();
  qsort(qsort_array, length, sizeof(int), sort_compare);
  double time_spent_qsort = omp_get_wtime() - begin;
  fprintf(qsort_stats, "%fs %d %d %d\n", time_spent_qsort, length, chunk_size, threads);
  fclose(qsort_stats);

  free(merge_array);
  free(qsort_array);
  return 0;
}

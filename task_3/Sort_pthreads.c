#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "thread_pool.h"


typedef struct {
  int* arr;
  int* buffer;
  size_t length;
} SortTask;

static size_t chunk_size = 64;

void set_chunk_size(size_t size) {
  chunk_size = size;
}

int sort_compare(const void* a, const void* b) {
  int a_val = *((const int*)a);
  int b_val = *((const int*)b);
  return a_val - b_val;
}

void task_destroyer(void* in) {
  ThreadPool* pool = in;
  ThreadPoolShutdown(pool);
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

void task_sorter(void* in) {
  SortTask* task = in;
  if (task->length <= chunk_size) {
    qsort(task->arr, task->length, sizeof(int), sort_compare);
  } 
  else {
    merge(task->arr, task->buffer, task->length / 2,
          task->length - task->length / 2);
  }
  free(task);
}


ThreadTask* task_builder(ThreadPool* pool, int* arr, int* buffer, size_t length) {
  ThreadTask* task = malloc(sizeof(ThreadTask));
  SortTask* data = malloc(sizeof(SortTask));

  data->arr = arr;
  data->buffer = buffer;
  data->length = length;

  ThreadPoolCreateTask(task, data, task_sorter);

  if (length >= chunk_size) {
    size_t left_size = length / 2;
    size_t right_size = length - left_size;

    ThreadTask* left_task = task_builder(pool, arr, buffer, left_size);
    ThreadTask* right_task = task_builder(pool, arr + left_size, buffer + left_size, right_size);

    ThreadPoolSetDependant(left_task, task);
    ThreadPoolSetDependant(right_task, task);
  } 
  else {
    ThreadPoolAddTask(pool, task);
  }

  return task;
}



double merge_sort(int* arr, size_t length, size_t threads) {
  int* buffer = malloc(sizeof(int) * length);
  ThreadPool thread_pool;
  ThreadTask* end_task = malloc(sizeof(ThreadTask));
  ThreadTask* sort_task;
  struct timeval time_begin;
  struct timeval time_end;
  gettimeofday(&time_begin, NULL);
  ThreadPoolInit(&thread_pool, threads);

  ThreadPoolCreateTask(end_task, &thread_pool, task_destroyer);
  sort_task = task_builder(&thread_pool, arr, buffer, length);
  ThreadPoolSetDependant(sort_task, end_task);
  ThreadPoolStart(&thread_pool);
  ThreadPoolJoin(&thread_pool);

  ThreadPoolDestroy(&thread_pool);
  free(buffer);
  gettimeofday(&time_end, NULL);
  return ((time_end.tv_sec - time_begin.tv_sec) * 1e6 + (time_end.tv_usec - time_begin.tv_usec)) / 1.0e6;
}


void print_buf(int* arr, size_t count, FILE* file) {
  size_t i;
  for (i = 0; i < count; i++) {
    fprintf(file, "%d ", arr[i]);
  }
  fprintf(file, "\n");
}

int main(int argc, char* argv[]) {
  int* array_merge;
  int* array_qsort;
  size_t length;
  size_t chunk_size;
  int threads;
  double time_spent_merge;
  double time_spent_qsort;

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
  

  array_merge = malloc(sizeof(int) * length);
  array_qsort = malloc(sizeof(int) * length);
  
  size_t i;
  for (i = 0; i < length; i++) {
    array_merge[i] = rand() % 16;
  }
  memcpy(array_qsort, array_merge, sizeof(int) * length);
  print_buf(array_merge, length, data);

  set_chunk_size(chunk_size);
  time_spent_merge = merge_sort(array_merge, length, threads);
  print_buf(array_merge, length, data);
  fclose(data);
  fprintf(stats, "%fs %d %d %d\n", time_spent_merge, length, chunk_size, threads);
  fclose(stats);
  
  struct timeval begin;
  struct timeval end;
  gettimeofday(&begin, NULL);
  qsort(array_qsort, length, sizeof(int), sort_compare);
  gettimeofday(&end, NULL);
  time_spent_qsort = ((end.tv_sec - begin.tv_sec) * 1e6 + (end.tv_usec - begin.tv_usec)) / 1.0e6;
  fprintf(qsort_stats, "%fs %d %d %d\n", time_spent_qsort, length, chunk_size, threads);
  fclose(qsort_stats);



  free(array_merge);
  free(array_qsort);
  return 0;
}

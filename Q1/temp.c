/* C program for Merge Sort */
#define _POSIX_C_SOURCE 199309L //required for clock
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>

int * shareMem(size_t size){
     key_t mem_key = IPC_PRIVATE;
     int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
     return (int*)shmat(shm_id, NULL, 0);
}

void merge(int *arr, int l, int m, int r)
{
	int i, j, k;
	int n1 = m - l + 1;
	int n2 = r - m;


	int L[n1], R[n2];


	for (i = 0; i < n1; i++)
		L[i] = arr[l + i];
	for (j = 0; j < n2; j++)
		R[j] = arr[m + 1 + j];


	i = 0;
	j = 0;
	k = l;
	while (i < n1 && j < n2) {
		if (L[i] <= R[j]) {
			arr[k] = L[i];
			i++;
		}
		else {
			arr[k] = R[j];
			j++;
		}
		k++;
	}

	while (i < n1) {
		arr[k] = L[i];
		i++;
		k++;
	}

	while (j < n2) {
		arr[k] = R[j];
		j++;
		k++;
	}
}


void mergeSort(int *arr, int l, int r)
{
	if (l < r){

		int m = l + (r - l) / 2;

        pid_t leftChild = fork();
        if(leftChild < 0) perror("fork: ");
		// Sort left half
        if(leftChild == 0){
		    mergeSort(arr, l, m);
            exit(0);
        }

        pid_t rightChild = fork();
        if(rightChild < 0) perror("fork: ");
        // Sort right half
        if(rightChild == 0){
		    mergeSort(arr, m + 1, r);
            exit(0);
        }

        // merge them in parent process!!
        if((leftChild > 0) && (rightChild > 0)){
            waitpid(leftChild,NULL,WUNTRACED);
            waitpid(rightChild,NULL,WUNTRACED);
            merge(arr, l, m, r);
        }
	}
    else{
        exit(0);
    }
}

void printArray(int A[], int size)
{
	int i;
	for (i = 0; i < size; i++)
		printf("%d ", A[i]);
	printf("\n");
}

int main()
{
    int n;
    scanf("%d",&n);

    int *arr = shareMem(sizeof(int)*(7+1));
    for(int i=0;i<n;i++){
        scanf("%d",&arr[i]);
    }

	int arr_size = n;

	printf("Given array is \n");
	printArray(arr, arr_size);

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
    long double st = ts.tv_nsec/(1e9)+ts.tv_sec;

	mergeSort(arr, 0, arr_size - 1);

	clock_gettime(CLOCK_MONOTONIC, &ts);
    long double en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);

	printf("\nSorted array is \n");
	printArray(arr, arr_size);

    long double t1 = en-st;

	return 0;
}
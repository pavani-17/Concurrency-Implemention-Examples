#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>

#define ll long long int

ll * shareMem(size_t size){    
    // Shared memory between process for merge sort using child processes
    key_t mem_key = IPC_PRIVATE;
    int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
    if(shm_id == -1)
    {
        perror("Shared memory: ");
        exit(0);
    }
    // Identifier(shm_id) of the shared memory segment associated with the key
    // IPC_CREAT = Created the shared memory (Others are IPC_EXCL to check if already exists) 0666 - Memory permissions of the shared block
    // size = size of the shared memory

    // ** IPC_PRIVATE always ensures a unique key and thus, other processes will not be able to access (other outside processes)
    // Not private (Preferable name would be IPC_NEW)
    // ftok will create based on parameters, might be accessed by others
    return (ll*)shmat(shm_id, NULL, 0);
}


void merge(ll l, ll m, ll r, ll* arr)
{
    ll i,j,k;
    ll n1 = m - l + 1;
    ll n2 = r - m;

    ll L[n1], R[n2];

    for(i=0;i<n1;i++)
    {
        L[i] = arr[l+i];
    }
    for(j=0;j<n2;j++)
    {
        R[j] = arr[m+1+j];
    }

    i=0;
    j=0;
    k=l;

    while(i<n1 && j<n2)
    {
        if(L[i] < R[j])
        {
            arr[k] = L[i];
            i++;
            k++;
        }
        else
        {
            arr[k] = R[j];
            j++;
            k++;
        }
    }

    while(i<n1)
    {
        arr[k] = L[i];
        i++;
        k++;
    }

    while(j<n2)
    {
        arr[k] = R[j];
        j++;
        k++;
    }
}

void mergeSort (ll l, ll r, ll *arr)
{
    if(l<r)
    {
        ll m = l + (r-l)/2;
        mergeSort(l,m,arr);
        mergeSort(m+1,r,arr);
        merge(l,m,r,arr);
    }
}

void mergeSort_process(ll l, ll r, ll *arr)
{
    if(l<r)
    {
        ll m = l + (r-l)/2;
        pid_t left_proc = fork();
        if(left_proc < 0)
        {
            perror("Child process");
            exit(0);
        }
        if(left_proc==0)
        {
            mergeSort_process(l,m,arr);
            exit(0);
        }
        else
        {
            pid_t right_proc = fork();
            if(right_proc<0)
            {
                perror("Child process");
                exit(0);
            }

            if(right_proc==0)
            {
                mergeSort_process(m+1,r,arr);
                exit(0);
            }
            else
            {
                waitpid(left_proc,NULL,WUNTRACED);
                waitpid(right_proc,NULL,WUNTRACED);
                merge(l,m,r,arr);
            }
        }
    }
    return;
}

void main()
{
    ll n;
    printf("Enter the number of elements in the array:\n");
    scanf("%lld",&n);
    ll arr[n];
    ll *arr_proc = shareMem(sizeof(ll)*(n+5));
    printf("Enter the array elements:\n");
    ll i;
    for(i=0;i<n;i++)
    {
        scanf("%lld",&arr[i]);
        arr_proc[i] = arr[i];
    }
    mergeSort_process(0,n-1,arr_proc);
    printf("Merge Sort by creating child process:\n");
    for(i=0;i<n;i++)
    {
        printf("%lld ",arr_proc[i]);
    }
    printf("\n");

    mergeSort(0,n-1,arr);
    printf("Normal Merge Sort\n");
    for(i=0;i<n;i++)
    {
        printf("%lld ",arr[i]);
    }
    printf("\n");

}
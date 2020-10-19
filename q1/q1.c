#define _POSIX_C_SOURCE 199309L //required for clock
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

struct arg 
{
    ll l;
    ll r;
    ll *arr;
};

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

void selectionSort (ll l, ll r, ll* arr)
{
    ll i,j;
    for(i=l;i<=r;i++)
    {
        for(j=i+1;j<=r;j++)
        {
            if(arr[i] > arr[j])
            {
                ll temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
    }
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
        if(r-l <= 5)
        {
            selectionSort(l,r,arr);
            return;
        }
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
        if(r-l <= 5)
        {
            selectionSort(l,r,arr);
            return;
        }
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

void *mergeSort_threaded (void *a)
{
    struct arg *args = (struct arg*) a;
    ll l = args->l;
    ll r = args->r;
    ll *arr = args->arr;
    if(l<r)
    {
        if(r-l <= 5)
        {
            selectionSort(l,r,arr);
            return NULL;
        }
        ll m = l + (r-l)/2;

        struct arg left_trd;
        left_trd.l = l;
        left_trd.r = m;
        left_trd.arr = arr;
        pthread_t left_id;
        
        struct arg right_trd;
        right_trd.l = m+1;
        right_trd.r = r;
        right_trd.arr = arr;
        pthread_t right_id;

        pthread_create(&left_id, NULL, mergeSort_threaded, &left_trd);
        pthread_create(&right_id, NULL, mergeSort_threaded, &right_trd);

        pthread_join(left_id,NULL);
        pthread_join(right_id,NULL);

        merge(l,m,r,arr);
    }
    return NULL;
}

void main()
{
    struct timespec ts;
    ll n;
    long double t1,t2,t3;
    printf("Enter the number of elements in the array:\n");
    scanf("%lld",&n);
    ll arr[n], arr_th[n];
    ll *arr_proc = shareMem(sizeof(ll)*(n+5)); // Creating shared memory for processes
    printf("Enter the array elements:\n");
    ll i;
    for(i=0;i<n;i++)
    {
        scanf("%lld",&arr[i]);
        arr_proc[i] = arr[i];
        arr_th[i] = arr[i];
    }
    
    printf("\n");

    clock_gettime(CLOCK_MONOTONIC_RAW,&ts);
    long double st = ts.tv_nsec/(1e9) + ts.tv_sec;
    mergeSort_process(0,n-1,arr_proc);
    clock_gettime(CLOCK_MONOTONIC_RAW,&ts);
    long double en = ts.tv_nsec/(1e9) + ts.tv_sec;
    printf("Merge Sort by creating child process:\n");
    for(i=0;i<n;i++)
    {
        printf("%lld ",arr_proc[i]);
    }
    printf("\n");
    printf("Time taken : %Lf \n\n",en-st);
    t1 = en - st;

    struct arg initial;
    initial.l=0;
    initial.r = n-1;
    initial.arr = arr_th;

    pthread_t int_id;
    clock_gettime(CLOCK_MONOTONIC_RAW,&ts);
    st = ts.tv_nsec/(1e9) + ts.tv_sec;
    pthread_create(&int_id,NULL,mergeSort_threaded,&initial);
    pthread_join(int_id,NULL);
    clock_gettime(CLOCK_MONOTONIC_RAW,&ts);
    en = ts.tv_nsec/(1e9) + ts.tv_sec;
    printf("Threaded Merge Sort\n");
    for(i=0;i<n;i++)
    {
        printf("%lld ",arr_th[i]);
    }
    printf("\n");
    printf("Time taken : %Lf \n\n",en-st);
    t2 = en - st;

    clock_gettime(CLOCK_MONOTONIC_RAW,&ts);
    st = ts.tv_nsec/(1e9) + ts.tv_sec;
    mergeSort(0,n-1,arr);
    clock_gettime(CLOCK_MONOTONIC_RAW,&ts);
    en = ts.tv_nsec/(1e9) + ts.tv_sec;
    printf("Normal Merge Sort\n");
    for(i=0;i<n;i++)
    {
        printf("%lld ",arr[i]);
    }
    printf("\n");
    printf("Time taken : %Lf \n\n",en-st);
    t3 = en - st;

    printf("For %lld elements:\n",n);
    printf("Normal merge sort                      : %Lf\n",t3);
    printf("Merge sort by creating child processes : %Lf\n",t1);
    printf("Threaded merge sort                    : %Lf\n",t2);
    printf("\n");
    printf("Normal merge sort ran:\n");;
    printf("\t \t \t [%Lf] times faster than threaded merge sort\n",t2/t3);
    printf("\t \t \t [%Lf] times faster than child process merge sort\n",t1/t3);
}
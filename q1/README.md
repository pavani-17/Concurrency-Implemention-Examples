# Merge Sort Parallelization

Merge Sort is a divide and conquer algorithm for sorting. It works by dividing the array into two halves, sorting the two halves and merging them. As the sorting of the two halves is independent, these two processes can be performed in parallel. In this program, we attempt to perform these parallel operations using child processes and threads, and compare the results to the normal merge sort algorithm. Note that the recursive merge sort algorithm is used in the program.

## Implementation

### Selection Sort

The program involves a segment of selection sort code which is used for all the 3 aforementioned merge sort algorithms when the number of elements is less than 5. This decreases the overhead of creating multiple threads or processes for sorting 5 elements. If done normally, the program creates around 10 threads/processes for sorting 5 elements. The following function is the selection sort algorithm called in all the 3 merge sorts: <br>
```
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
```
It sorts the elements of array `arr` from the postions `l` to `r`, both inclusive.

### Commom Merge Function

The merge function, used to combine the two sorted subarrays into one single sorted array, is used in all 3 functions.

```
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
```

### Normal Merge Sort
The normal recursive merge sort algorithm is used for comparision with multi-process and multi-threaded merge sort.

```
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
```

### Multi-Process Merge Sort
The multi-process merge sort creates two child processes in each function call (unless the number of elements is less than 5). Each of the child processes sorts one half of the array. The parent process combines both into a single array. Although the sorting happens in parallel, there is a large overhead in creating a new process, and this overcomes the benefit gained by the parallel sorting. Thus we observe that multi-process merge sort takes more time than normal merge sort.

```
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
```
To enable memory sharing between processes, we use the following function:
```
ll * shareMem(size_t size){    
    key_t mem_key = IPC_PRIVATE;
    int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
    if(shm_id == -1)
    {
        perror("Shared memory: ");
        exit(0);
    }
    return (ll*)shmat(shm_id, NULL, 0);
}
```
### Multi-Threaded Merge Sort

Similar to the multi-processes merge sort, the multi-threaded merge sort also sorts both the halves of the arrays in parallel. However, it does so by creating threads rather than processes. As threads share memory, the overhead is less than that of creating processes.

```
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
```

## Observation

The results of the program for a few values of n are as follows: <br>
 ```
 For 10 elements:
Normal merge sort                      : 0.000001
Merge sort by creating child processes : 0.000420
Threaded merge sort                    : 0.000277

Normal merge sort ran:
	 	 	 [202.412630] times faster than threaded merge sort
	 	 	 [306.494404] times faster than child process merge sort

 ```
<br>

```
For 1000 elements:
Normal merge sort                      : 0.000101
Merge sort by creating child processes : 0.027819
Threaded merge sort                    : 0.011477

Normal merge sort ran:
	 	 	 [113.803910] times faster than threaded merge sort
	 	 	 [275.846940] times faster than child process merge sort

```
<br>

```
For 100000 elements:
Normal merge sort                      : 0.016208
Merge sort by creating child processes : 3.316962
Threaded merge sort                    : 0.620638

Normal merge sort ran:
	 	 	 [38.291725] times faster than threaded merge sort
	 	 	 [204.647922] times faster than child process merge sort

```

We observe that as the number of elements in the array increases, the threaded merge sort shows drastic improvement with respect to the normal merge sort. This signifies that, when the number of elements is low, the overhead of the thread creation, which might not be scheduled in parallel, outweighs the benefits of parallel sorting. However, as the number of elements and thus sorting time increases, the parallel sorting benefit gradually becomes visible.
<br> <br>
However, process creation still consumes a lot of time as a creation of process has a higher overhead than that of threads which share a lot of data. Thus, the benefit of parallel sorting is only slightly visible.
<br> <br>
By increasing the number of elments on which selection sort is applied and thus decreasing the overhead for sorting fewer numbers, the parallelisation benefit can be realised as both the threaded and the multi-process merge sorts outperform the normal one.
<br> <br>
Selection sort for 1000 elements:

```
For 100000 elements:
Normal merge sort                      : 0.137376
Merge sort by creating child processes : 0.050850
Threaded merge sort                    : 0.040820

Normal merge sort ran:
	 	 	 [0.297141] times faster than threaded merge sort
	 	 	 [0.370154] times faster than child process merge sort

```
<br> 

Selection sort for 5000 elements:

```
For 100000 elements:
Normal merge sort                      : 0.496192
Merge sort by creating child processes : 0.154205
Threaded merge sort                    : 0.143037

Normal merge sort ran:
	 	 	 [0.288270] times faster than threaded merge sort
	 	 	 [0.310777] times faster than child process merge sort

```
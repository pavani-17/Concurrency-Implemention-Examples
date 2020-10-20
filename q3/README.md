# Musical Mayhem

This is a multithreaded simulation of the musicians performing in a college event. There are acoustic and electric stages. Musicians can perform on one or both of them. Singers can either perform solo or can join a musician in his performance. The simulation is implemented as follows:

## Implementation Details

### Musicians
Musicians are modelled as threads. Each musician has a data structure as follows:

```
typedef struct musician
{
    char name[500];
    int type;
    int mus_id;
    char inst_name;
    int arr_time;
    int stage_num;
    int mus_state;
    pthread_mutex_t mus_lock;
    pthread_mutex_t strong_lock;
    pthread_cond_t mus_cond;

} musician;
```
If the musician is eligible for only of the two stages, he executes timed_wait on a semaphore. If he gets a slot, he performs, else he returns. After getting a slot the musician searches for an empty stage in the following functions:

```

void getAcousticStage(musician* inp, int type)
{
    int i;
    for(i=0;i<a;i++)
    {
        if(st[i]->stage_state != STAGE_EMPTY)
        {
            continue;
        }
        pthread_mutex_lock(&(st[i]->stage_lock));
        if(st[i]->stage_state != STAGE_EMPTY)
        {
            pthread_mutex_unlock(&(st[i]->stage_lock));
            continue;
        }
        if(type==MUSICIAN)
        {
            st[i]->stage_state = STAGE_PERFORMING;
            inp->mus_state = MUS_PERFORMING;
            sem_post(&singer);
        }
        else
        {
            st[i]->stage_state = STAGE_FULL;
            inp->mus_state = MUS_PERFORMING;
        }
        st[i]->mus_id = inp->mus_id;
        inp->stage_num = i;
        pthread_mutex_unlock(&(st[i]->stage_lock));
        pthread_cond_signal(&(st[i]->stag_cond));
        return;
    }
}
```
A similar function exists for electric stages. <br>
However, for a musician eligible for both stages, two threads are created, each to wait on one semaphore. The one which returns first and before time will be choosen. If none of the threads gets a semaphore before the time elapses, the musician returns.

```
void* waitElectric (void* std)
{
    musician* inp = (musician*)std;
    sem_wait(&electric);
    if(pthread_mutex_trylock(&(inp->mus_lock)) == -1)
    {
        sem_post(&electric);
        return NULL;
    }
    if(inp->mus_state != MUS_WAITING)
    {
        sem_post(&electric);
        pthread_mutex_unlock(&(inp->mus_lock));
        return NULL;
    }
    inp->mus_state = MUS_ALLOCATED;
    inp->type = MUS_ELECTRIC;
    pthread_mutex_unlock(&(inp->mus_lock));
    pthread_cond_signal(&(inp->mus_cond));
    return NULL;
}
```
A similar function exists to wait on the acoustic stages. The wait functions signify a `conditional_timedwait()` after waiting on the semaphore. <br>
A singer is a special kind of musician, who can not only perform on both accoustic and electric stages, but also join a performing musician. To accomadate the singer, an extra semaphore is included, which keeps track of the number of performing musicians the singer can join. Similar to the musician eligible for both stages, the singer waits on three threads and selects the one which returns(signals) first. <br>

### Stages

In this simulation, stages are implemented as threads with the following data structure:
```
typedef struct stage
{
    int type;
    int stage_id;
    int stage_state;
    int time;
    int mus_id;
    int sing_id;
    pthread_mutex_t stage_lock;
    pthread_cond_t stag_cond;
}stage;
```

The stage waits for some musician to send it a signal. Then, it will check for any available singers who is looking to join a musician in his/her performance. It then signals the musician thread to allow it to pass on to the next step. The basic workflow of the stage is as follows:
```
while(1)
{
    Make_available()
    wait_for_signal()
    process_a_musician()
    search_for_singer()
}
```
Each musician and singer collects a T-Shirt in the end from the club coordinators. This is implemented using a semaphore to regulate entry to a fixed number of people.

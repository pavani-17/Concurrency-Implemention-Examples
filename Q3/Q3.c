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
#include <stdbool.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>

#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN "\033[1;36m"
#define NORMAL "\033[0m"

#define MUS_ACOUSTIC 0
#define MUS_ELECTRIC 1
#define MUS_BOTH 2
#define MUS_SINGER 3

#define STAGE_EMPTY 0
#define STAGE_PERFORMING 1
#define STAGE_FULL 2

#define MUS_NOTARRIVED 0
#define MUS_WAITING 1
#define MUS_ALLOCATED 2
#define MUS_LEFT 3

int t1,t2,t,a,e;
sem_t singer,club_cord,ac_cnt,el_cnt;

typedef struct musician
{
    char name[500];
    int type;
    int mus_id;
    char inst_name;
    int per_time;
    int stage_num;
    int mus_state;
    pthread_mutex_t mus_lock;
    pthread_cond_t mus_cond;
} musician;


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

stage** st;
musician** m;

void* stages(void* std)
{
    stage* inp = (stage*)std;
    while(1)
    {
        pthread_mutex_lock(&(inp->stage_lock));
        inp->stage_state = STAGE_EMPTY;
        if(inp->type == MUS_ELECTRIC)
        {
            sem_post(&ac_cnt);
        }
        else 
        {
            sem_post(&el_cnt);
        }
        pthread_cond_wait(&(inp->stag_cond),&(inp->stage_lock));
        pthread_mutex_unlock(&(inp->stage_lock));
        pthread_mutex_lock(&m[inp->mus_id]->mus_lock);
        // if(inp->stage_state==STAGE_FULL)
        // {
        //     sleep(inp->time);
        //     printf("%s completed performance on stage %d\n",m[inp->mus_id]->name,inp->stage_id);
        //     inp->stage_state = STAGE_EMPTY;
        //     pthread_mutex_unlock(&(inp->stage_lock));
        // }
        //else
        {
            struct timespec ts; 
            if(clock_gettime(CLOCK_REALTIME,&ts)==-1)
            {
                perror("gettime ");
                return NULL;
            }
            ts.tv_sec += inp->time;
            sleep(inp->time);
            printf("%s completed perfomance on stage %d of type %d\n",m[inp->mus_id]->name,inp->stage_id,inp->type);
            pthread_mutex_unlock(&m[inp->mus_id]->mus_lock);
            pthread_cond_signal(&(m[inp->mus_id]->mus_cond));
            
        }
            // pthread_cond_timedwait(&(inp->stag_cond),&(inp->stage_lock),&ts);
            // if(inp->stage_state==STAGE_FULL)
            // {
            //     struct timespec en; 
            //     if(clocl_gettime(CLOCK_REALTIME,&en)==-1)
            //     {
            //         perror("gettime ");
            //         return NULL;
            //     }
            //     ts.tv_nsec -= inp->time;
            //     sleep(en.tv_sec-ts.tv_sec+2);
            //     printf("%s and %s completed performance on stage %d",m[inp->mus_id]->name,m[inp->sing_id]->name,inp->stage_id);
            //     inp->stage_state = STAGE_EMPTY;
            //     pthread_mutex_unlock(&(inp->stage_lock));
            //     sem_post(&sing_stage);
            // }
            // else
            // {
            //     printf("%s completed performance at stage %d\n",m[inp->mus_id]->name,inp->stage_id);
            //     inp->stage_state = STAGE_EMPTY;
            //     pthread_mutex_unlock(&(inp->stage_lock));
            // }
    }
    return NULL;
}

void getAcousticStage(musician* inp)
{
    int i;
    // While loop compensates for the trylock.
    // As trylock does not wait to get the control, if a context switch occurs exactly before the conditional wait, the trylock will not be able to lock it
    // Then, the system will not be able to find the acoustic stage even though it is available (the semaphore enters only if it is available)
    // Try with normal lock later
    while(1)
    {
        for(i=0;i<a;i++)
        {
            if(pthread_mutex_trylock(&(st[i]->stage_lock))==0)            
            {
                if(st[i]->stage_state != STAGE_EMPTY)
                {
                    pthread_mutex_unlock(&(st[i]->stage_lock));
                    continue;
                }
                st[i]->stage_state = STAGE_PERFORMING;
                st[i]->time = rand()%(t2-t1+1) + t1;
                st[i]->mus_id = inp->mus_id;
                printf("Musician %s is starting performance on acoustic stage %d for time %d\n",inp->name,st[i]->stage_id,st[i]->time);
                pthread_mutex_unlock(&(st[i]->stage_lock));
                pthread_cond_signal(&(st[i]->stag_cond));
                return;
            }
        }
    }
}

void getElectricStage(musician* inp)
{
    int i;
    while(1) 
    {
        for(i=a;i<a+e;i++)
        {
            if(pthread_mutex_trylock(&(st[i]->stage_lock))==0)
            {
                if(st[i]->stage_state != STAGE_EMPTY)
                {
                    pthread_mutex_unlock(&(st[i]->stage_lock));
                    continue;
                }
                st[i]->stage_state = STAGE_PERFORMING;
                st[i]->time = rand()%(t2-t1+1) + t1;
                st[i]->mus_id = inp->mus_id;
                printf("Musician %s is starting performance on electric stage %d for time %d\n",inp->name,st[i]->stage_id,st[i]->time);
                pthread_mutex_unlock(&(st[i]->stage_lock));
                pthread_cond_signal(&(st[i]->stag_cond));
                return;
            }
        }
    }
}



void* waitAcoustic(void* std)
{
    int mus_id = *(int*)std;
    sem_wait(&ac_cnt);
    pthread_mutex_lock(&m[mus_id]->mus_lock);
    if(m[mus_id]->mus_state != MUS_WAITING)
    {
        sem_post(&ac_cnt);
        pthread_mutex_unlock(&m[mus_id]->mus_lock);
    }
    else
    {
        m[mus_id]->mus_state = MUS_ALLOCATED;
        m[mus_id]->type = MUS_ACOUSTIC;
        pthread_mutex_unlock(&m[mus_id]->mus_lock);
        pthread_cond_signal(&(m[mus_id]->mus_cond));
    }
    
    return NULL;
}

void* waitElectric(void* std)
{
    int mus_id = *((int*)std);
    sem_wait(&el_cnt);
    pthread_mutex_lock(&m[mus_id]->mus_lock);
    if(m[mus_id]->mus_state != MUS_WAITING)
    {
        sem_post(&el_cnt);
        pthread_mutex_unlock(&m[mus_id]->mus_lock);
    }
    else
    {
        m[mus_id]->mus_state = MUS_ALLOCATED;
        m[mus_id]->type = MUS_ACOUSTIC;
        pthread_mutex_unlock(&m[mus_id]->mus_lock);
        pthread_cond_signal(&(m[mus_id]->mus_cond));
    }
    return NULL;
}

void setTime(struct timespec *ts)
{
    if(clock_gettime(CLOCK_REALTIME,ts)==-1)
    {
        perror("gettime ");
        return ;
    }
    ts->tv_sec += t;
    ts->tv_nsec = 0;
}

void* musicians(void* std)
{
    musician* inp = (musician*)std;
    sleep(inp->per_time);
    inp->mus_state = MUS_WAITING;
    switch (inp->inst_name)
    {
        case 'p':
        case 'g':
                inp->type = MUS_BOTH;
                break;

        case 'v':
                inp->type = MUS_ACOUSTIC;
                break;

        case 'b':
                inp->type = MUS_ELECTRIC;
                break;
        case 's':
                inp->type = MUS_SINGER;
    }
    struct timespec ts;
    
    if(inp->type == MUS_ACOUSTIC)
    {
        if(clock_gettime(CLOCK_REALTIME,&ts)==-1)
        {
            perror("gettime ");
            return NULL;
        }
        ts.tv_sec += t;
        ts.tv_nsec = 0;
        if(sem_timedwait(&ac_cnt,&ts)==-1 && errno == ETIMEDOUT)
        {
            printf("%s could not get a stage\n",inp->name);
            return NULL;
        }
        getAcousticStage(inp);
        pthread_mutex_lock(&inp->mus_lock);
        pthread_cond_wait(&(inp->mus_cond),&(inp->mus_lock));
    }
    else if(inp->type == MUS_ELECTRIC)
    {
        if(clock_gettime(CLOCK_REALTIME,&ts)==-1)
        {
            perror("gettime ");
            return NULL;
        }
        ts.tv_sec += t;
        ts.tv_nsec = 0;
        if(sem_timedwait(&el_cnt,&ts)==-1 && errno == ETIMEDOUT)
        {
            printf("%s could not get a stage\n",inp->name);
            return NULL;
        }
        getElectricStage(inp);
        pthread_mutex_lock(&inp->mus_lock);
        pthread_cond_wait(&(inp->mus_cond),&(inp->mus_lock));
    }
    else if(inp->type == MUS_BOTH)
    {
        pthread_mutex_lock(&inp->mus_lock);
        pthread_t ac_wait, el_wait;
        pthread_create(&ac_wait,NULL,waitAcoustic,(void*)&(inp->mus_id));
        pthread_create(&ac_wait,NULL,waitElectric,(void*)&(inp->mus_id));
        if(clock_gettime(CLOCK_REALTIME,&ts)==-1)
        {
            perror("gettime ");
            return NULL;
        }
        ts.tv_sec += t;
        ts.tv_nsec = 0;
        pthread_cond_timedwait(&(inp->mus_cond),&(inp->mus_lock),&ts);
        if(inp->mus_state == MUS_WAITING)
        {
            inp->mus_state = MUS_LEFT;
            printf("%s could not get a stage\n",inp->name);
            pthread_mutex_unlock(&inp->mus_lock);
            return NULL;
        }
        pthread_mutex_unlock(&inp->mus_lock);

        if(inp->type == MUS_ACOUSTIC)
        {
            getAcousticStage(inp);
        }
        else
        {
            getElectricStage(inp);
        }
        pthread_mutex_lock(&inp->mus_lock);
        pthread_cond_wait(&(inp->mus_cond),&(inp->mus_lock));
    }

    sem_wait(&club_cord);
    printf("Musician %s collecting T-Shirt\n",inp->name);
    sleep(2);
    sem_post(&club_cord);
}

void main()
{
    int k,c;
    printf("Enter the number of perfomers, acoustic stages, electric stages,club coordinators, minimum and maximum time of perfomance and time of patience:\n");
    scanf("%d %d %d %d %d %d %d",&k,&a,&e,&c,&t1,&t2,&t);
    int i;
    sem_init(&ac_cnt,0,0);
    sem_init(&el_cnt,0,0);
    sem_init(&club_cord,0,c);
    pthread_t sta[a+e], mus[k];
    st = malloc((a+e)*sizeof(stage*));
    char name_inp[k][500];
    char type_inp[k][2];
    int time_inp[k];
    printf("Enter the musician details\n");
    for(i=0;i<k;i++)
    {
        scanf("%s",name_inp[i]);
        scanf("%s",type_inp[i]);
        scanf("%d",&time_inp[i]);
        //printf("%s %s %d\n",name_inp[i], type_inp[i], time_inp[i]);
    }
    
    for(i=0;i<a;i++)
    {
        st[i] = malloc(sizeof(stage));
        st[i]->type = MUS_ACOUSTIC;
        st[i]->stage_id = i;
        st[i]->stage_state = STAGE_EMPTY;
        pthread_mutex_init(&(st[i]->stage_lock),NULL);
        pthread_cond_init(&(st[i]->stag_cond),NULL);
        pthread_create(&sta[i],NULL,stages,(void*)st[i]);
    }

    for(i=0;i<e;i++)
    {
        st[i+a] = malloc(sizeof(stage));
        st[i+a]->type = MUS_ELECTRIC;
        st[i+a]->stage_id = i;
        st[i+a]->stage_state = STAGE_EMPTY;
        pthread_mutex_init(&(st[i]->stage_lock),NULL);
        pthread_cond_init(&(st[i]->stag_cond),NULL);
        pthread_create(&sta[i],NULL,stages,(void*)st[i]);
    }
    m = malloc(k*sizeof(musician));
    for(i=0;i<k;i++)
    {
        m[i] = malloc(sizeof(musician));
        strcpy(m[i]->name,name_inp[i]);
        m[i]->per_time = time_inp[i];
        m[i]->inst_name = type_inp[i][0];
        m[i]->mus_id = i;
        m[i]->mus_state = MUS_NOTARRIVED;
        pthread_create(&mus[i],NULL,musicians,(void*)m[i]);
    }
    for(i=0;i<k;i++)
    {
        pthread_join(mus[i],NULL);
    }
}

// void getStage_singer(musician* inp)
// {
//     int i;

//     for(i=0;i<a;i++)
//     {
//         if(pthread_mutex_trylock(&(st[i]->stage_lock))==0)
//         {
//             if(st[i]->stage_state == STAGE_FULL)
//             {
//                 pthread_mutex_unlock(&(st[i]->stage_lock));
//                 continue;
//             }
//             if(st[i]->stage_state == STAGE_EMPTY)
//             {
//                 st[i]->stage_state = STAGE_FULL;
//                 st[i]->time = rand()%3;
//                 st[i]->mus_id = inp->mus_id;
//                 printf("Singer %s is performing on acoustic stage %d for time %d\n",inp->name,st[i]->stage_id,st[i]->time);
//                 pthread_mutex_unlock(&(st[i]->stage_lock));
//                 pthread_cond_signal(&st[i]->stag_cond);  
//             }
//             else
//             {
//                 st[i]->stage_state = STAGE_FULL;
//                 st[i]->sing_id = inp->mus_id;
//                 printf("Singer %s joined %s on acoustic stage %d\n",inp->name,m[st[i]->mus_id]->name,st[i]->stage_id);
//                 pthread_mutex_unlock(&(st[i]->stage_lock));
//                 pthread_cond_signal(&st[i]->stag_cond);
//             }
//             return;
            
//         }
//     }
//     for(i=a;i<a+e;i++)
//     {
//         if(pthread_mutex_trylock(&(st[i]->stage_lock))==0)
//         {
//             if(st[i]->stage_state == STAGE_FULL)
//             {
//                 pthread_mutex_unlock(&(st[i]->stage_lock));
//                 continue;
//             }
//             if(st[i]->stage_state == STAGE_EMPTY)
//             {
//                 st[i]->stage_state = STAGE_FULL;
//                 st[i]->time = rand()%3;
//                 st[i]->mus_id = inp->mus_id;
//                 printf("Singer %s is performing on electric stage %d for time %d\n",inp->name,st[i]->stage_id,st[i]->time);
//                 pthread_cond_signal(&st[i]->stag_cond);
//                 pthread_mutex_unlock(&(st[i]->stage_lock));    
//             }
//             else
//             {
//                 st[i]->stage_state = STAGE_FULL;
//                 st[i]->sing_id = inp->mus_id;
//                 printf("Singer %s joined %s on acoustic stage %d\n",inp->name,m[st[i]->mus_id]->name,st[i]->stage_id);
//                 pthread_mutex_unlock(&(st[i]->stage_lock));
//                 pthread_cond_signal(&st[i]->stag_cond);
//             }
//             return;
//         }
//     }
// }
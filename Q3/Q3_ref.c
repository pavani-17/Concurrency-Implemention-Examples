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

#define MUS_NOTARRIVED 5
#define MUS_WAITING 1
#define MUS_ALLOCATED 2
#define MUS_LEFT 3

int t,a,e,t1,t2;
sem_t acoustic, electric, club_cord;

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
        if(inp->type == MUS_ACOUSTIC)
        {
            sem_post(&acoustic);
        }
        else
        {
            sem_post(&electric);
        }
        pthread_cond_wait(&(inp->stag_cond),&(inp->stage_lock));
        pthread_mutex_unlock(&(inp->stage_lock));
        pthread_mutex_lock(&(m[inp->mus_id]->mus_lock));
        inp->time = t1 + rand()%(t2-t1+1);
        sleep(inp->time);
        printf("Musician %s completed playing on %s stage %d\n",m[inp->mus_id]->name,inp->type==MUS_ACOUSTIC ? "Acoustic" : "Electric",inp->stage_id);
        pthread_mutex_unlock(&(m[inp->mus_id]->mus_lock));
        pthread_cond_signal(&(m[inp->mus_id]->mus_cond));
    }
    return NULL;
}

void getAcousticStage(musician* inp)
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
        st[i]->stage_state = STAGE_PERFORMING;
        printf("Musician %s is starting performance on acoustic stage %d\n",inp->name,st[i]->stage_id);
        st[i]->mus_id = inp->mus_id;
        inp->stage_num = i;
        pthread_mutex_unlock(&(st[i]->stage_lock));
        pthread_cond_signal(&(st[i]->stag_cond));
        return;
    }
}

void getElectricStage(musician* inp)
{
    int i;
    for(i=a;i<a+e;i++)
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
        st[i]->stage_state = STAGE_PERFORMING;
        printf("Musician %s is starting performance on electric stage %d\n",inp->name,st[i]->stage_id);
        st[i]->mus_id = inp->mus_id;
        inp->stage_num = i;
        pthread_mutex_unlock(&(st[i]->stage_lock));
        pthread_cond_signal(&(st[i]->stag_cond));
        return;
    }
}

void* waitAcoustic (void* std)
{
    musician* inp = (musician*)std;
    sem_wait(&acoustic);
    pthread_mutex_lock(&(inp->mus_lock));
    if(inp->mus_state != MUS_WAITING)
    {
        sem_post(&acoustic);
        pthread_mutex_unlock(&(inp->mus_lock));
        return NULL;
    }
    inp->mus_state = MUS_ALLOCATED;
    inp->type = MUS_ACOUSTIC;
    pthread_mutex_unlock(&(inp->mus_lock));
    pthread_cond_signal(&(inp->mus_cond));
    return NULL;
}

void* waitElectric (void* std)
{
    musician* inp = (musician*)std;
    sem_wait(&electric);
    pthread_mutex_lock(&(inp->mus_lock));
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

void* musicians(void* std)
{
    musician* inp = (musician*)std;
    sleep(inp->arr_time);
    printf("Musician %s arrived\n",inp->name);
    inp->mus_state = MUS_WAITING;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME,&ts);
    ts.tv_sec += t;
    if(inp->type == MUS_ACOUSTIC)
    {
        if(sem_timedwait(&acoustic,&ts)==-1 && errno == ETIMEDOUT)
        {
            printf("%s did not get a stage.\n",inp->name);
            return NULL;
        }
        pthread_mutex_lock(&(inp->mus_lock));
        getAcousticStage(inp);
        pthread_cond_wait(&(inp->mus_cond),&(inp->mus_lock));
    }
    else if(inp->type == MUS_ELECTRIC)
    {
        if(sem_timedwait(&electric,&ts)==-1 && errno == ETIMEDOUT)
        {
            printf("%s did not get a stage.\n",inp->name);
            return NULL;
        }
        pthread_mutex_lock(&(inp->mus_lock));
        getElectricStage(inp);
        pthread_cond_wait(&(inp->mus_cond),&(inp->mus_lock));
    }
    else if(inp->type == MUS_BOTH)
    {
        pthread_t acc,ele;
        pthread_mutex_lock(&(inp->mus_lock));
        pthread_create(&acc,NULL,waitAcoustic,(void*)inp);
        pthread_create(&ele,NULL,waitElectric,(void*)inp);
        pthread_cond_timedwait(&(inp->mus_cond),&(inp->mus_lock),&ts);
        if(inp->mus_state == MUS_WAITING)
        {
            printf("%s could not get a stage\n",inp->name);
            return NULL;
        }
        if(inp->type == MUS_ACOUSTIC)
        {
            getAcousticStage(inp);
        }
        else
        {
            getElectricStage(inp);
        }
        pthread_cond_wait(&(inp->mus_cond),&(inp->mus_lock));
    }
    sem_wait(&club_cord);
    printf("Musician %s collecting T-Shirt\n",inp->name);
    sleep(2);
    sem_post(&club_cord);
}

int main()
{
    int k,c,i;
    printf("Enter k,a,e,c,t1,t2 and t:\n");
    scanf("%d %d %d %d %d %d %d",&k,&a,&e,&c,&t1,&t2,&t);
    
    char mus_name[k][500];
    char mus_inst[k][2];
    int mus_arrtime[k];

    printf("Enter the nusician details:\n");
    for(i=0;i<k;i++)
    {
        scanf("%s %s %d",mus_name[i],mus_inst[i],&mus_arrtime[i]);
    }

    sem_init(&club_cord,0,c);
    sem_init(&acoustic,0,0);
    sem_init(&electric,0,0);

    st = malloc((a+e)*sizeof(stage*));
    pthread_t stage_thread[a+e], mus_thread[k];
    for(i=0;i<a;i++)
    {
        st[i] = malloc(sizeof(stage));
        st[i]->stage_state = STAGE_FULL;
        st[i]->type = MUS_ACOUSTIC;
        st[i]->stage_id = i;
        pthread_mutex_init(&st[i]->stage_lock,NULL);
        pthread_cond_init(&st[i]->stag_cond,NULL);
        pthread_create(&stage_thread[i],NULL,stages,(void*)st[i]);
    }
    for(i=a;i<a+e;i++)
    {
        st[i] = malloc(sizeof(stage));
        st[i]->stage_state = STAGE_FULL;
        st[i]->type = MUS_ELECTRIC;
        st[i]->stage_id = i;
        pthread_mutex_init(&st[i]->stage_lock,NULL);
        pthread_cond_init(&st[i]->stag_cond,NULL);
        pthread_create(&stage_thread[i],NULL,stages,(void*)st[i]);
    }

    m = malloc(k*sizeof(musician*));
    for(i=0;i<k;i++)
    {
        m[i] = malloc(sizeof(musician));
        m[i]->arr_time = mus_arrtime[i];
        m[i]->mus_id = i;
        m[i]->mus_state = MUS_NOTARRIVED;
        strcpy(m[i]->name,mus_name[i]);
        m[i]->inst_name = mus_inst[i][0];
        pthread_mutex_init(&m[i]->mus_lock,NULL);
        pthread_cond_init(&m[i]->mus_cond,NULL);
        switch (m[i]->inst_name)
        {
            case 'p':
            case 'g':
                    m[i]->type = MUS_BOTH;
                    break;

            case 'v':
                    m[i]->type = MUS_ACOUSTIC;
                    break;

            case 'b':
                    m[i]->type = MUS_ELECTRIC;
                    break;
            case 's':
                    m[i]->type = MUS_SINGER;
        }
        pthread_create(&mus_thread[i],NULL,musicians,(void*)m[i]);
    }
    for(i=0;i<k;i++)
    {
        pthread_join(mus_thread[i],NULL);
    }
    return 0;
}


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


#define RED "\033[3;31m"
#define GREEN "\033[3;32m"
#define YELLOW "\033[3;33m"
#define BLUE "\033[3;34m"
#define MAGENTA "\033[3;35m"
#define CYAN "\033[3;36m"
#define NORMAL "\033[0m"


# define NYA 0
# define WTP 1
# define ATP 7
# define PSo 2
# define PSi 3
# define WTT 4
# define TSC 5
# define Ex 6

# define FR 0
# define M 1
# define S 2
# define MS 3

#define TYPE_ACOUSTIC 1
#define TYPE_ELECTRIC 2
#define TYPE_BOTH 3
#define TYPE_SINGER 4

typedef struct musician{
int id;
pthread_t tid;
char name[100];
int arrtime;
int stage_id;
int type;
int status;
int play_time;
pthread_mutex_t mutex_race;
pthread_cond_t cond_mutex;
}musician;


typedef struct stage{
int id;
int status;
pthread_mutex_t mutex;
int type;
int musician;
int singer;
sem_t sem;
}stage;


typedef struct coordinator{
int id;
pthread_t tid;
pthread_mutex_t mutex;
}coordinator;

musician ** MUSICIAN;
stage ** STAGE;
coordinator ** COORDINATOR;

typedef struct waiter{
musician * player;
pthread_t tid;
int type;
}waiter;

musician ** MUSICIAN;
stage ** STAGE;
coordinator ** COORDINATOR;

int num_player;
int num_stage;
// int num_coordinator;

sem_t acoustic;
sem_t electric;
sem_t singer;
sem_t both;
sem_t coordinate;

int k,a,e,c,t1,t2,t;


///////////////////////////////////////////////////////////////////////////////////////////////

void* singerTshirt(void* temp)
{
    musician* self = (musician*) temp;
    self->status = WTT;
    sem_wait(&coordinate);
    sleep(2);
    printf(CYAN"%s collected T-Shirt\n"NORMAL,self->name);
    self->status=TSC;
    sem_post(&coordinate);
    self->status=Ex;
}

void postPerformance(musician * self){
    int sing_partner=-1;
    if(self->status == PSi){
        sleep(2);
        sing_partner = STAGE[self->stage_id]->singer;
    }
    else{
        int val = sem_trywait(&singer);
        if(val==-1)
        {
            sleep(2);
            sing_partner = STAGE[self->stage_id]->singer;
            
        }
    }

    if(sing_partner==-1) printf(GREEN"%s is done performing \n"NORMAL,self->name); 
    else printf(GREEN"%s and %s done performing\n"NORMAL,MUSICIAN[sing_partner]->name,self->name);

    // free the stage
    pthread_mutex_lock(&(STAGE[self->stage_id]->mutex));
    STAGE[self->stage_id]->status = FR;
    STAGE[self->stage_id]->singer = -1;   
    STAGE[self->stage_id]->musician = -1;  
    pthread_mutex_unlock(&(STAGE[self->stage_id])->mutex);


    // free the semaphores
    if(STAGE[self->stage_id]->type == TYPE_ACOUSTIC) sem_post(&acoustic);
    else sem_post(&electric);

    pthread_t tc;

    if(sing_partner != -1)
    {
        pthread_create(&tc, NULL, singerTshirt,(void*)MUSICIAN[sing_partner]);
    }

    // go to collect t-shirts!
    self->status = WTT;
    sem_wait(&coordinate);
    sleep(2);
    printf(CYAN"%s collected T-Shirt\n"NORMAL,self->name);
    self->status=TSC;
    sem_post(&coordinate);
    self->status=Ex;
    if(sing_partner!=-1)  pthread_join(tc,NULL);
}


void getStage(int type, musician * player){

    for(int i=0;i<a+e;i++){
        pthread_mutex_lock(&(STAGE[i]->mutex));
        if(STAGE[i]->type==type){
            if(STAGE[i]->status==FR){
                // setting stage

                if(player->type == TYPE_SINGER)STAGE[i]->status = S;
                else STAGE[i]->status = M;
                STAGE[i]->musician = player->id;    // Note: musician == non 2nd

                // setting player(non 2nd)

                player->stage_id = i;
                player->status = PSo;

                printf(YELLOW"%s starting perfomance on %s stage %d for %d secs\n"NORMAL,player->name,STAGE[i]->type == TYPE_ACOUSTIC ? "acoustic" : "electric",player->stage_id,player->play_time);
                if(player->type != TYPE_SINGER)  sem_post(&singer);  // free semaphore for singer

                pthread_mutex_unlock(&(STAGE[i]->mutex));
                return;
            }
        }
        pthread_mutex_unlock(&(STAGE[i]->mutex));
    }
}


void getSinger(musician * player){
    for(int i=0;i<a+e;i++){
        pthread_mutex_lock(&(STAGE[i]->mutex));

        if(STAGE[i]->status == M){  // only if a musician is playing!!

        // setting the stage
            STAGE[i]->status = MS;
            STAGE[i]->singer = player->id;

        // setting the other musician
            MUSICIAN[STAGE[i]->musician]->status= PSi;
            player->status = PSi;
            player->stage_id = i;

            printf(BLUE"Singer %s joined %s on %s stage %d. Exceeding time by 2 secs!\n"NORMAL,player->name, MUSICIAN[STAGE[i]->musician]->name,STAGE[i]->type == TYPE_ACOUSTIC ? "acoustic" : "electric",player->stage_id);
            pthread_mutex_unlock(&(STAGE[i]->mutex));
            return;

        }
        pthread_mutex_unlock(&(STAGE[i]->mutex));
    }
}

void * waitStage(void * temp){
    waiter * self = (waiter *)temp;
    musician * player = self->player;
    int type = self->type;
    struct timespec ts;
    if(clock_gettime(CLOCK_REALTIME,&ts)==-1){
        perror("gettime ");
        return NULL;
    }
    ts.tv_sec += t;
    ts.tv_nsec = 0;

    int semRet;
    if(self->type == TYPE_ACOUSTIC) semRet = sem_timedwait(&acoustic,&ts);
    else if(self->type == TYPE_ELECTRIC) semRet = sem_timedwait(&electric,&ts);
    else semRet = sem_timedwait(&singer,&ts);

    pthread_mutex_lock(&player->mutex_race); // locked

    if(player->status!=WTP){
        pthread_mutex_unlock(&player->mutex_race);
        if(semRet!=-1){ // decreased the semaphore, so increase now
            if(self->type == TYPE_ACOUSTIC) sem_post(&acoustic);
            else if(self->type == TYPE_ELECTRIC) sem_post(&electric);
            else sem_post(&singer);
        }
        return NULL;
    }
    else{
        player->status = ATP;
        if(semRet == -1)
        {
            printf(RED"%s did not get a stage and left impatiently.\n"NORMAL,player->name);
            pthread_mutex_unlock(&player->mutex_race);
            return NULL;
        }
        
    }

    pthread_mutex_unlock(&player->mutex_race);    //unlocked

    if(self->type!=TYPE_SINGER){
        getStage(type,player);
        sleep(player->play_time);
        postPerformance(player);
    }
    else getSinger(player);

    return NULL;
}



void * musician_procedure(void * temp){

    musician * self = (musician *)temp;
    sleep(self->arrtime);
    self->status = WTP;

    printf(CYAN"%s arrived to srujana\n"NORMAL,self->name);


    if(self->type == TYPE_ACOUSTIC){

        waiter * waitAc;
        waitAc = malloc(sizeof(waiter));
        waitAc->type = TYPE_ACOUSTIC;
        waitAc->player = self;
        pthread_create(&(waitAc->tid),NULL,waitStage,(void *)waitAc);
        pthread_join(waitAc->tid,NULL);

    }

    else if(self->type == TYPE_ELECTRIC){
        waiter * waitEl;
        waitEl = malloc(sizeof(waiter));
        waitEl->type = TYPE_ELECTRIC;
        waitEl->player = self;
        pthread_create(&(waitEl->tid),NULL,waitStage,(void *)waitEl);
        pthread_join(waitEl->tid,NULL);

    }

    else if(self->type == TYPE_BOTH){   // both

        waiter * waitAc;
        waitAc = malloc(sizeof(waiter));
        waitAc->type = TYPE_ACOUSTIC;
        waitAc->player = self;

        waiter * waitEc;
        waitEc = malloc(sizeof(waiter));
        waitEc->type = TYPE_ELECTRIC;
        waitEc->player = self;

        pthread_create(&(waitAc->tid),NULL,waitStage,(void *)waitAc);
        pthread_create(&(waitEc->tid),NULL,waitStage,(void *)waitEc);

        pthread_join(waitAc->tid,NULL);
        pthread_join(waitEc->tid,NULL);

    }
    else{ // singer

        waiter * waitAc;
        waitAc = malloc(sizeof(waiter));
        waitAc->type = TYPE_ACOUSTIC;
        waitAc->player = self;

        waiter * waitEc;
        waitEc = malloc(sizeof(waiter));
        waitEc->type = TYPE_ELECTRIC;
        waitEc->player = self;

        waiter * waitS;
        waitS = malloc(sizeof(waiter));
        waitS->type = TYPE_SINGER;
        waitS->player = self;

        pthread_create(&(waitAc->tid),NULL,waitStage,(void *)waitAc);
        pthread_create(&(waitEc->tid),NULL,waitStage,(void *)waitEc);
        pthread_create(&(waitS->tid),NULL,waitStage,(void *)waitS);

        pthread_join(waitAc->tid,NULL);
        pthread_join(waitEc->tid,NULL);
        pthread_join(waitS->tid,NULL);

        // NOTE if its a 2nd mem, then it will simply die here after getting stage, but the other musician will take care of it!

    }

}

int main(){
    printf("Enter k,a,e,c,t1,t2,t\n");
    scanf("%d %d %d %d %d %d %d",&k,&a,&e,&c,&t1,&t2,&t);

    sem_init(&acoustic,0,a);
    sem_init(&electric,0,e);
    sem_init(&coordinate,0,c);

    STAGE = (stage **)malloc(sizeof(stage *) * (a+e));
    MUSICIAN = (musician **)malloc(sizeof(musician *) * (k));

    for(int i=0;i<a+e;i++){
        STAGE[i] = malloc(sizeof(stage));
        if(i<a)STAGE[i]->type = TYPE_ACOUSTIC;
        else STAGE[i]->type = TYPE_ELECTRIC;

        STAGE[i]->id = i;
        STAGE[i]->status = FR;
        STAGE[i]->musician = -1;
        STAGE[i]->singer = -1;
        pthread_mutex_init(&(STAGE[i]->mutex),NULL);
    }
    printf("Enter the musician details--name,ins,arrtime\n");
    for(int i=0;i<k;i++){
        MUSICIAN[i] = malloc(sizeof(musician));
        scanf("%s",MUSICIAN[i]->name);
        char ins[2];
        scanf("%s",ins);
        if(ins[0] == 'g' || ins[0] == 'p') MUSICIAN[i]->type = TYPE_BOTH;
        else if(ins[0] == 'v')MUSICIAN[i]->type = TYPE_ACOUSTIC;
        else if(ins[0] == 'b')MUSICIAN[i]->type = TYPE_ELECTRIC;
        else MUSICIAN[i]->type = TYPE_SINGER;

        MUSICIAN[i]->status = NYA;
        MUSICIAN[i]->id = i;
        MUSICIAN[i]->play_time = rand()%(t2-t1+1)+t1;

        pthread_mutex_init(&(MUSICIAN[i]->mutex_race),NULL);
        pthread_cond_init(&(MUSICIAN[i]->cond_mutex),NULL);
        scanf("%d",&(MUSICIAN[i]->arrtime));
    }
    printf("\n-----------------------------------------------------STARTING SIMULATION---------------------------------------------------------\n\n");
    for(int i=0;i<k;i++){
        pthread_create(&(MUSICIAN[i]->tid),NULL,musician_procedure,(void *)MUSICIAN[i]);
    }

    for(int i=0;i<k;i++){
        pthread_join((MUSICIAN[i]->tid),NULL);
    }
    printf("\n\n-----------------------------------------------------ENDING SIMULATION---------------------------------------------------------\n");
    return 0;
}
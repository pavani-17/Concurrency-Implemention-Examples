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

#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN "\033[1;36m"
#define NORMAL "\033[0m"

int n,m,o;
int rem_stud;
pthread_mutex_t lock_rem_stud;

typedef struct pharma_company
{
    int pharma_id;
    int time_sec;
    int no_batches;
    bool ready;
    int ind_size;
    float vac_prob;
    pthread_mutex_t comp_lock;
    pthread_cond_t comp_cond;
    int use;
} pharma_company;

typedef struct vaccination_zone
{
    int zone_id;
    int pharma_comp_id;
    int batch_size;
    int rem;
    int slots_given;
    bool ready;
    bool* stud_done;
    pthread_cond_t zone_cond;
    pthread_mutex_t zone_lock;
} vaccination_zone;

typedef struct student
{
    int student_id;
    int vac_zone_id;
    int status;
} student;

int min (int a, int b)
{
    if(a<b)
    {
        return a;
    }
    return b;
}

pharma_company** pc;
vaccination_zone** vz;
student** sd;

void allotCompany(pharma_company* inp)
{
    inp->ready = true;
    while(1)
    {
        if(inp->no_batches<=0)
        {
            break;
        }
        else
        {
            pthread_cond_wait(&(inp->comp_cond),&(inp->comp_lock));
        }
    }
    inp->ready = false;
    pthread_mutex_unlock(&(inp->comp_lock));
    printf(MAGENTA"Distributed all vaccines of Pharmaceutical company %d, waiting for them to be used to resume distribution\n"NORMAL,inp->pharma_id);
}

void* pharma_companies (void* str)
{
    pharma_company* inp = (pharma_company*)str;
    while(1)
    {
        pthread_mutex_lock(&(inp->comp_lock));
        inp->time_sec = rand()%3 + 2;
        inp->no_batches = rand()%5 + 1;
        inp->ind_size = rand()%10 + 11;
        inp->use = inp->no_batches;
        printf(CYAN"Pharmaceutical Company %d is preparing %d batches of vaccines of size %d with success probability %f\n"NORMAL,inp->pharma_id,inp->no_batches,inp->ind_size,inp->vac_prob);
        sleep(inp->time_sec);
        printf(GREEN"Pharmaceutical Company %d manufactured %d batches of size %d with success probability %f\n"NORMAL,inp->pharma_id,inp->no_batches,inp->ind_size,inp->vac_prob);
        allotCompany(inp);
        while(inp->use>0);
        printf(RED"All distributed vaccines have been used. Pharmaceutical Company %d resuming production\n",inp->pharma_id);
    }   
}

void getCompany(vaccination_zone* inp)
{
    printf(MAGENTA"Vaccination Zone %d waiting for a batch of vaccines\n"NORMAL,inp->zone_id);
    int i;
    while(1)
    {
        for(i=0;i<m;i++)
        {
            if(pthread_mutex_trylock(&(pc[i]->comp_lock))==0)
            {
                if(pc[i]->ready == false || pc[i]->no_batches==0)
                {
                    pthread_mutex_unlock(&(pc[i]->comp_lock));
                    pthread_cond_signal(&(pc[i]->comp_cond));
                    continue;
                }
                inp->pharma_comp_id = i;
                inp->batch_size = pc[i]->ind_size;
                printf(GREEN"Vaccination Zone %d got a batch of size %d and success probability %f from Pharmaceutical Company %d\n"NORMAL,inp->zone_id,inp->batch_size,pc[i]->vac_prob,inp->pharma_comp_id);
                pc[i]->no_batches--;
                pthread_mutex_unlock(&(pc[i]->comp_lock));
                pthread_cond_signal(&(pc[i]->comp_cond));
                inp->rem = inp->batch_size;
                return ;
            }
        }
    }
}

void allotZone(vaccination_zone* inp)
{
    printf(CYAN"Vaccination Zone %d allocating %d slots for next phase of vaccination\n"NORMAL,inp->zone_id,inp->slots_given);
    pthread_mutex_lock(&(inp->zone_lock));
    inp->ready = true;
    while(1)
    {
        if(inp->slots_given<=0)
        {
            break;
        }
        else
        {
            pthread_cond_wait(&(inp->zone_cond),&(inp->zone_lock));
        }
    }
    printf(YELLOW"Vaccination Zone %d entering into vaccination phase\n"NORMAL,inp->zone_id);
    inp->ready = false;
    pthread_mutex_unlock(&(inp->zone_lock));

}

void* vaccination_zones(void* std)
{
    vaccination_zone* inp = (vaccination_zone*)std;
    int i;
    while(1)
    {    
        getCompany(inp);
        while(inp->rem!=0)
        {
            pthread_mutex_lock(&(lock_rem_stud));
            if(rem_stud==0)
            {
                pthread_mutex_unlock(&(lock_rem_stud));
                continue;
            }
            inp->slots_given = rand()%(min(8,min(rem_stud,inp->rem))) + 1;
            inp->rem = inp->rem - inp->slots_given;
            rem_stud = rem_stud - inp->slots_given;
            pthread_mutex_unlock(&(lock_rem_stud));
            allotZone(inp);
            for(i=0;i<o;i++)
            {
                while(inp->stud_done[i]);
            }
            printf(GREEN"Vaccination Zone %d has completed the vaccination phase\n",inp->zone_id);
        }
        printf(RED"Vaccination Zone %d has run out of vaccines\n"NORMAL,inp->zone_id);
        pthread_mutex_lock(&(pc[inp->pharma_comp_id]->comp_lock));
        pc[inp->pharma_comp_id]->use--;
        pthread_mutex_unlock(&(pc[inp->pharma_comp_id]->comp_lock));
    }
}

void getZone(student* inp)
{
    printf(YELLOW"Student %d is waiting to be allocated a vaccination zone\n"NORMAL,inp->student_id);
    int i;
    while(1)
    {
        for(i=0;i<n;i++)
        {
            if(pthread_mutex_trylock(&(vz[i]->zone_lock))==0)
            {
                if(vz[i]->ready==false || vz[i]->slots_given<=0)
                {
                    pthread_mutex_unlock(&(vz[i]->zone_lock));
                    pthread_cond_signal(&(vz[i]->zone_cond));
                    continue;
                }
                inp->vac_zone_id = i;
                vz[i]->stud_done[inp->student_id] = true;
                printf(BLUE"Student %d alloted a slot on the Vaccination Zone and is waiting to be vaccinated %d\n"NORMAL,inp->student_id,inp->vac_zone_id);
                vz[i]->slots_given--;
                pthread_mutex_unlock(&(vz[i]->zone_lock));
                pthread_cond_signal(&(vz[i]->zone_cond));
                return;
            }
        }
    }
}

void* students(void* std)
{
    student* inp = (student*)std;
    printf(CYAN"Student %d has arrived for Round %d of Vaccination\n"NORMAL,inp->student_id,inp->status);
    sleep(rand()%2);
    while(1)
    {
        getZone(inp);
        while(vz[inp->vac_zone_id]->slots_given>0);
        printf(GREEN"Student %d on Vaccination Zone %d has been vaccinated with vaccine which has success probability %f\n"NORMAL,inp->student_id,inp->vac_zone_id,pc[vz[inp->vac_zone_id]->pharma_comp_id]->vac_prob);
        vz[inp->vac_zone_id]->stud_done[inp->student_id] = false;
        sleep(rand()%3);
        printf(MAGENTA"Student %d proceeding to antibody check\n"NORMAL,inp->student_id);
        float prob = (float)rand()/RAND_MAX;
        if(prob < pc[vz[inp->vac_zone_id]->pharma_comp_id]->vac_prob)
        {
            printf(GREEN"Student %d has tested positive for antibodies and is going to college :)\n"NORMAL,inp->student_id);
            return NULL;
        }
        else
        {
            inp->status++;
            if(inp->status==4)
            {
                printf(RED"Student %d has completed 3 rounds of vaccination and tested negative in all 3. Is sent home for another online semester :(\n"NORMAL,inp->student_id);
                return NULL;
            }
            printf(RED"Student %d tested negative for antibodies. Proceeding to next round\n"NORMAL,inp->student_id);
            pthread_mutex_lock(&(lock_rem_stud));
            rem_stud++;
            pthread_mutex_unlock(&(lock_rem_stud));
        }
        
    }
}

void main()
{
    int i;
    printf("Enter the number of pharmaceutical companies, vaccination zones and students: \n");
    scanf("%d %d %d",&m,&n,&o);
    pthread_mutex_init(&(lock_rem_stud),NULL);
    float prob[m];
    printf("Enter the probabilities of each of the pharmaceutical companies:\n");
    for(i=0;i<m;i++)
    {
        scanf("%f",&prob[i]);
    }
    rem_stud = o;
    pthread_t ph_comp[m];
    pc = malloc(m*sizeof(pharma_company*));
    for(i=0;i<m;i++)
    {
        pc[i] = (pharma_company*)malloc(sizeof(pharma_company));
        pc[i]->pharma_id=i;
        pc[i]->ready=false;
        pc[i]->vac_prob = prob[i];
        pthread_mutex_init(&(pc[i]->comp_lock),NULL);
        pthread_cond_init(&(pc[i]->comp_cond),NULL);
        pthread_create(&ph_comp[i],NULL,pharma_companies,(void*)pc[i]);
    }
    pthread_t v_zone[n];
    vz = malloc(n*sizeof(vaccination_zone*));
    for(i=0;i<n;i++)
    {
        vz[i] = (vaccination_zone*)malloc(sizeof(vaccination_zone));
        vz[i]->zone_id = i;
        vz[i]->ready = false;
        pthread_mutex_init(&(vz[i]->zone_lock),NULL);
        pthread_cond_init(&(vz[i]->zone_cond),NULL);
        vz[i]->stud_done = malloc(o*sizeof(bool));
        int j;
        for(j=0;j<o;j++)
        {
            vz[i]->stud_done[j] = false;
        }
        pthread_create(&v_zone[i],NULL,vaccination_zones,(void*)vz[i]);
    }
    pthread_t stud[o];
    sd = malloc(o*sizeof(student*));
    for(i=0;i<o;i++)
    {
        sd[i] = (student*)malloc(sizeof(student));
        sd[i]->student_id = i;
        sd[i]->status = 1;
        pthread_create(&stud[i],NULL,students,(void*)sd[i]);
    }

    for(i=0;i<o;i++)
    {
        pthread_join(stud[i],NULL);
    }
    printf("Done Simulation\n");
}
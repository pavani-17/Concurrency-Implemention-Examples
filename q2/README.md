# Back to College

This program is a multithreaded simulation of a vaccination process, involving pharmaceutical companies for manufacturing the vaccine, vaccination zones for the slots allocation and students who get vaccinated. The solution uses mutex locks and busy waiting for synchronisation.

## Implementation

The simulation includes a thread for each pharmaceutical company, vaccination zone and student involved. The details are described below:

### Pharmaceutical company

A pharmaceutical company takes `w` time (random between 2 to 5) to prepare `r` batches (random between 1 to 5) of vaccines, each capable of vaccinating `p` students (random between 10 to 20). These batches have a probability of success `p` which is fixed for a company.<br>
The data structure used for pharmaceutical company is as follows:
```
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
```
The pharmaceutical company distributes all of its vaccines and waits till all of them are used to start the production of a new batch. The pharmaceutical company thread starts execution from the following function (printf statements omitted for clarity):

```
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
        sleep(inp->time_sec);
        allotCompany(inp);
        while(inp->use>0);
    }   
}
```
The `allotCompany()` dispatches all the different batches to the respective vaccination zones.
```
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
}
```
`while(inp->use > 0)` is a busy waiting loop which ensures all the vaccines are used before starting the production of new ones.

### Vaccination Zones

A vaccination zone receives a batch of vaccines from the company. It then makes `k` slots available for vaccination. `k <= min(8, remaining student, remaining vaccines). Then, it allocates these slots and enters into the vaccination phase. A vaccination zone cannot allot slots when it is in vaccination phase. <br>
The data structure used for the vaccination zone is as follows:
```
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
```
The function from which the vaccination zone thread starts execution is as follows:
```
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
        }
        pthread_mutex_lock(&(pc[inp->pharma_comp_id]->comp_lock));
        pc[inp->pharma_comp_id]->use--;
        pthread_mutex_unlock(&(pc[inp->pharma_comp_id]->comp_lock));
    }
}
```
`getCompany()` function fetches a batch of vaccines for the vaccination zone. The vaccination zone keeps on checking all the pharmaceutical companies till it gets a batch of vaccines (Busy Waiting).
```
void getCompany(vaccination_zone* inp)
{
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
                pc[i]->no_batches--;
                pthread_mutex_unlock(&(pc[i]->comp_lock));
                pthread_cond_signal(&(pc[i]->comp_cond));
                inp->rem = inp->batch_size;
                return ;
            }
        }
    }
}
```
After the vaccination zone receives a batch of vaccines, it starts allocating slots. This continues till the vaccination zone runs out of vaccines. It allocates the slots using `allotZone()` function. This function uses busy waiting to allocate all the slots.
```
void allotZone(vaccination_zone* inp)
{
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
    inp->ready = false;
    pthread_mutex_unlock(&(inp->zone_lock));
}
```
After allocation, the vaccination zone busy waits on a boolean variable to check if all students have been vaccinated.

### Students
Students arrive and wait for slots in a vaccination zone. After being alloted a slot, it waits for the zone to enter into vaccination phase. After being vaccinated, it proceeds to an antibosy check. If positive, the student is done. Else, he re-enters for the next round. A student can have 3 rounds of vaccination before he is deemed inelligible for further rounds.
The data structure of the student is as follows:
```
typedef struct student
{
    int student_id;
    int vac_zone_id;
    int status;
} student;
```
The student thread starts vaccination from the following function:
```
void* students(void* std)
{
    student* inp = (student*)std;
    printf(CYAN"Student %d has arrived for Round %d of Vaccination\n"NORMAL,inp->student_id,inp->status);
    sleep(rand()%2);
    while(1)
    {
        getZone(inp);
        while(vz[inp->vac_zone_id]->slots_given>0);
        vz[inp->vac_zone_id]->stud_done[inp->student_id] = false;
        sleep(rand()%3);
        float prob = (float)rand()/RAND_MAX;
        if(prob < pc[vz[inp->vac_zone_id]->pharma_comp_id]->vac_prob)
        {
            return NULL;
        }
        else
        {
            inp->status++;
            if(inp->status==4)
            {
                return NULL;
            }
            pthread_mutex_lock(&(lock_rem_stud));
            rem_stud++;
            pthread_mutex_unlock(&(lock_rem_stud));
        }
    }
}
```
In `getZone()` function, the student busy waits on all vacciation zones till it finds an available slot.
```
void getZone(student* inp)
{
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
                vz[i]->slots_given--;
                pthread_mutex_unlock(&(vz[i]->zone_lock));
                pthread_cond_signal(&(vz[i]->zone_cond));
                return;
            }
        }
    }
}
```
After getting a slot, the student busy waits till its parent vaccination zone allots all of it slots in the current vaccination phase. Then, it gets vaccinated and proceeds to antibody check. If failed, it repeats the same process again for a maximum of 3 times.

### Remaining Students:
The remaining students at any given point of time is stored in a mutex lock protected variable `rem_stud`.

## Sample Case:
The execution of the code for a sample test case of `1 2 3  0.5` is as follows:
```
Starting simulation
Pharmaceutical Company 0 is preparing 2 batches of vaccines of size 18 with success probability 0.500000
Vaccination Zone 0 waiting for a batch of vaccines
Student 0 has arrived for Round 1 of Vaccination
Student 1 has arrived for Round 1 of Vaccination
Vaccination Zone 1 waiting for a batch of vaccines
Student 2 has arrived for Round 1 of Vaccination
Student 0 is waiting to be allocated a vaccination zone
Student 1 is waiting to be allocated a vaccination zone
Student 2 is waiting to be allocated a vaccination zone
Pharmaceutical Company 0 manufactured 2 batches of size 18 with success probability 0.500000
Vaccination Zone 1 got a batch of size 18 and success probability 0.500000 from Pharmaceutical Company 0
Vaccination Zone 0 got a batch of size 18 and success probability 0.500000 from Pharmaceutical Company 0
Vaccination Zone 1 allocating 2 slots for next phase of vaccination
Distributed all vaccines of Pharmaceutical company 0, waiting for them to be used to resume distribution
Student 1 alloted a slot on the Vaccination Zone and is waiting to be vaccinated 1
Student 0 alloted a slot on the Vaccination Zone and is waiting to be vaccinated 1
Student 1 on Vaccination Zone 1 has been vaccinated with vaccine which has success probability 0.500000
Vaccination Zone 1 entering into vaccination phase
Student 0 on Vaccination Zone 1 has been vaccinated with vaccine which has success probability 0.500000
Vaccination Zone 1 has completed the vaccination phase
Vaccination Zone 0 allocating 1 slots for next phase of vaccination
Student 2 alloted a slot on the Vaccination Zone and is waiting to be vaccinated 0
Student 2 on Vaccination Zone 0 has been vaccinated with vaccine which has success probability 0.500000
Vaccination Zone 0 entering into vaccination phase
Vaccination Zone 0 has completed the vaccination phase
Student 1 proceeding to antibody check
Student 1 tested negative for antibodies. Proceeding to next round
Student 1 is waiting to be allocated a vaccination zone
Vaccination Zone 1 allocating 1 slots for next phase of vaccination
Student 1 alloted a slot on the Vaccination Zone and is waiting to be vaccinated 1
Student 1 on Vaccination Zone 1 has been vaccinated with vaccine which has success probability 0.500000
Vaccination Zone 1 entering into vaccination phase
Vaccination Zone 1 has completed the vaccination phase
Student 0 proceeding to antibody check
Student 0 tested negative for antibodies. Proceeding to next round
Student 0 is waiting to be allocated a vaccination zone
Vaccination Zone 1 allocating 1 slots for next phase of vaccination
Student 0 alloted a slot on the Vaccination Zone and is waiting to be vaccinated 1
Student 0 on Vaccination Zone 1 has been vaccinated with vaccine which has success probability 0.500000
Vaccination Zone 1 entering into vaccination phase
Vaccination Zone 1 has completed the vaccination phase
Student 1 proceeding to antibody check
Student 1 tested negative for antibodies. Proceeding to next round
Student 1 is waiting to be allocated a vaccination zone
Vaccination Zone 1 allocating 1 slots for next phase of vaccination
Student 1 alloted a slot on the Vaccination Zone and is waiting to be vaccinated 1
Student 1 on Vaccination Zone 1 has been vaccinated with vaccine which has success probability 0.500000
Vaccination Zone 1 entering into vaccination phase
Student 0 proceeding to antibody check
Student 0 has tested positive for antibodies and is going to college :)
Vaccination Zone 1 has completed the vaccination phase
Student 2 proceeding to antibody check
Student 2 has tested positive for antibodies and is going to college :)
Student 1 proceeding to antibody check
Student 1 has tested positive for antibodies and is going to college :)
Done Simulation
```
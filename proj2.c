/**
 * @author Albert Tikaiev 
 * LOGIN : xtikaia00
 * 
 * CODE STYLE
 * Arguments    -> UPPERCASE
 * Functions    -> camelCase
 * Semaphores   -> flatcase
 * Shared mem   -> snake_case
 * 
 * @file proj2.c
 * @brief VUT FIT IOS PROJECT №2
*/
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

//Output selectors for skibus
#define STARTED_BUS 1
#define ARRIVED_BUS 2
#define LEAVING_BUS 3
#define FINARR_BUS 4
#define FINLEAV_BUS 5
#define FINISH_BUS 6

//Output selectors for skier
#define STARTED_SKIER 7
#define ARRIVED_SKIER 8
#define BOARDING_SKIER 9
#define GOING_SKIER 10

//Functions
void writeFile(int case_write, int idL, int idZ);
void destMemory();
void initMemory();
void errMemory();
void destSems();
void initSems();
void errSem();
void skierProcess(int Id, int stop);
void skibusProcess();
void parseArguments(int argumentc, char**argumentv);

//Arguments
int L;  //Amount of skierProcesss <20,000
int Z;  //Amount of stops 0<Z<=10
int K;  //Capacity of bus 10<=K<=100
int TL; //0<=TL<=10,000
int TB; //0<=TB<=1,000

//Semaphores
sem_t **allowboarding = NULL;   //Semaphores for skiers process, for specific stops, so skiers can come in
sem_t *allboarded = NULL;       //Semaphore for bus process, controls if all skiers have already came in or got out
sem_t *destination = NULL;      //Semaphore for skiers process, controls if skiers can go skiing (final destination)
sem_t *mutex = NULL;            //Mutex semaphore
sem_t* writingfile = NULL;      //Writing file semaphore
sem_t* busfinish = NULL;        //Main process waiting semaphore

//Shared memory(variables)
int* stops_skiers = NULL;       //Array of amounts of skiers waiting for the bus
int* current_capacity = NULL;   //Current capacity of the bus
int* skiers_left = NULL;        //How many skiers waiting for bus
int* output_line = NULL;        //Controls the number of line in output file

//Shared memory(variables) for fork error
int* forked_skiers = NULL;      //How many skier processes was forked
int* fork_err = NULL;           //Boolean int | 0 -> no fork error | 1 -> fork error

FILE* file = NULL;

/**
 * @brief WRITE TEXT IN FILE
 * 
 * @param case_write Which text will write to file
 * @param idL Skier id
 * @param idZ Stop id
*/
void writeFile(int case_write, int idL, int idZ){
    sem_wait(writingfile);
    (*output_line)++;
    switch(case_write){
        //bus part
        case STARTED_BUS:
            fprintf(file,"%i: BUS: started\n", *output_line);
            break;
        case ARRIVED_BUS:
            fprintf(file,"%i: BUS: arrived to %i\n", *output_line, idZ);
            break;
        case LEAVING_BUS:
            fprintf(file,"%i: BUS: leaving %i\n", *output_line, idZ);
            break;
        case FINARR_BUS:
            fprintf(file,"%i: BUS: arrived to final\n", *output_line);
            break;
        case FINLEAV_BUS:
            fprintf(file,"%i: BUS: leaving final\n", *output_line);
            break;
        case FINISH_BUS:
            fprintf(file,"%i: BUS: finish\n", *output_line);
            break;
        //skier part
        case STARTED_SKIER:
            fprintf(file,"%i: L %i: started\n", *output_line, idL);
            break;
        case GOING_SKIER:
            fprintf(file,"%i: L %i: going to ski\n", *output_line, idL);
            break;
        case ARRIVED_SKIER:
            fprintf(file,"%i: L %i: arrived to %i\n", *output_line, idL, idZ);
            break;
        case BOARDING_SKIER:
            fprintf(file,"%i: L %i: boarding\n", *output_line, idL);
            break;
    }
    fflush(file);
    sem_post(writingfile);
}

/**
 * @brief ERROR MEMORY
*/
void errMemory(){
    fprintf(stderr, "Failed with mapping memory\n");
    destMemory();
    fclose(file);
    exit(1);
}

/**
 * @brief INITIALIZE SHARED MEMORY
*/
void initMemory(){
    //Semaphores mapping
    if((allowboarding = mmap(NULL, sizeof(sem_t*)*Z, PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) == MAP_FAILED) errMemory();
    for(int i = 0; i<Z;i++){
        if((allowboarding[i] = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) ==MAP_FAILED) errMemory();
    }
    if((allboarded = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) ==MAP_FAILED) errMemory();
    if((destination = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) ==MAP_FAILED) errMemory();
    if((mutex = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) ==MAP_FAILED) errMemory();
    if((writingfile = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) ==MAP_FAILED) errMemory();
    if((busfinish = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) ==MAP_FAILED) errMemory();
    //Shared memory mapping
    if((stops_skiers = mmap(NULL, sizeof(int)*Z, PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) ==MAP_FAILED) errMemory();
    if((current_capacity = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) ==MAP_FAILED) errMemory();
    if((skiers_left = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) ==MAP_FAILED) errMemory();
    if((output_line = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) ==MAP_FAILED) errMemory();
    //Shared memory mapping (fork error)
    if((forked_skiers = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) ==MAP_FAILED) errMemory();
    if((fork_err = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0)) ==MAP_FAILED) errMemory();
}

/**
 * @brief FREE SHARED MEMORY
*/
void destMemory(){
    //Unmap semaphores
    for(int i = 0; i< Z;i++){
        munmap(allowboarding[i], sizeof(sem_t));
    }
    munmap(allowboarding, sizeof(sem_t*)*Z);
    munmap(mutex, sizeof(sem_t));
    munmap(writingfile, sizeof(sem_t)); 
    munmap(allboarded, sizeof(sem_t)); 
    munmap(destination, sizeof(sem_t)); 
    munmap(busfinish, sizeof(sem_t)); 
    //Unmap shared memory
    munmap(stops_skiers, sizeof(int)*Z);
    munmap(current_capacity, sizeof(int));
    munmap(skiers_left, sizeof(int)); 
    munmap(output_line, sizeof(int));
    //Unmap shared memory(fork error)
    munmap(forked_skiers, sizeof(int)); 
    munmap(fork_err, sizeof(int));
}

/**
 * @brief ERROR SEMAPHORES
*/
void errSem(){
    fprintf(stderr, "Failed with semaphore initialize\n");
    destSems();
    destMemory();
    fclose(file);
    exit(1);
}

/**
 * @brief INITIALIZE SEMAPHORES
*/
void initSems(){
    if(sem_init(mutex, 1, 1) == -1) errSem();
    if(sem_init(writingfile, 1,1) == -1) errSem();
    for(int i = 0; i<Z;i++){
        if(sem_init(allowboarding[i], 1, 0) == -1) errSem();
    }
    if(sem_init(allboarded, 1, 0) == -1) errSem();
    if(sem_init(destination, 1, 0) == -1) errSem();
    if(sem_init(busfinish, 1, 0) == -1) errSem();
}

/**
 * @brief DESTROY ALL SEMAPHORES
*/
void destSems(){
    sem_destroy(mutex);
    sem_destroy(writingfile);
    for(int i = 0; i<Z;i++){
        sem_destroy(allowboarding[i]); 
    }
    sem_destroy(allboarded);
    sem_destroy(destination);
    sem_destroy(busfinish);
}

/**
 * @brief SKIERS PROCESS
 * 
 * @param Id id of skier (not process id)
 * @param stop on which stop skier must stay
*/
void skierProcess(int Id, int stop){
    //***************INITIALIZING***************

    srand(time(0) * Id );       //Random time
    writeFile(STARTED_SKIER, Id, -1);
    usleep(rand()%(TL+1));      //Eating
    
    sem_wait(mutex);            //Next skier cant change counters

    (*forked_skiers)++;         //Increase the count of forked skiers
    stops_skiers[stop-1]+=1;    //Add this skier to the count of skiers on his stop
    writeFile(ARRIVED_SKIER, Id, stop);

    sem_post(mutex);            //Next skier can change counters

    //***************BOARDING***************
    
    sem_wait(allowboarding[stop-1]);//Skier waits till bus comes to his stop

    writeFile(BOARDING_SKIER, Id, -1);

    sem_wait(mutex);            //Next skier cant change counters

    stops_skiers[stop-1]-=1;    //Decrease the count of skiers on this stop
    (*skiers_left)--;           //Decrease the count of skiers that havent boarded yet
    (*current_capacity)++;      //Increase current capacity of skibus

    sem_post(mutex);            //Next skier can change counters

    //All skiers came to bus or there isn't enough capacity
    if(stops_skiers[stop-1] == 0 || *current_capacity ==K) sem_post(allboarded); //Dont let next skier in and let skibus continue
    else sem_post(allowboarding[stop-1]);//Let next skier in

    //***************LEAVING***************
    sem_wait(destination);      //Skier waits until final destination

    //Getting out of the bus
    sem_wait(mutex);            //Next skier can't change counter

    writeFile(GOING_SKIER, Id, -1);
    (*current_capacity)--;      //Decrease current bus capacity
    (*forked_skiers)--;         //Decrease the count of forked skier

    if(*current_capacity == 0) sem_post(allboarded); //All skiers got out from bus | Only one check at time(mutex block)
    else sem_post(destination); //Let next skier out from bus

    sem_post(mutex);            //Next skier can change counter or skibus can continue with opened mutex
    exit(0);
}

/**
 * @brief SKIBUS PROCESS
*/
void skibusProcess(){
    srand(time(0) * getpid());
    writeFile(STARTED_BUS, -1, -1);
    int bus_stop = 0;

    while(1){
        //***************RIDING***************

        bus_stop=(bus_stop+1)%(Z+1);    //Change stop
        usleep(rand()%(TB+1));          //Riding to the next stop

        //***************FINAL STOP ARRIVING***************

        if(bus_stop == 0) { 
            writeFile(FINARR_BUS, -1, -1);
            if(*current_capacity > 0)sem_post(destination);  //Allow skiers get out
            if(*current_capacity == 0) sem_post(allboarded); //No skiers on bus | Immediately continue to first stop
        }

        //***************REGULAR STOP ARRIVING***************
        else {
            writeFile(ARRIVED_BUS, -1, bus_stop);

            //Stops if there is somebody on the stop and the capacity isn't full
            sem_wait(mutex);

            if(stops_skiers[bus_stop-1] != 0 && *current_capacity != K) {
                sem_post(mutex);
                sem_post(allowboarding[bus_stop-1]); 
                sem_wait(allboarded);
            }
            else sem_post(mutex);
        }

        //***************FINAL STOP LEAVING***************

        if(bus_stop == 0){
            sem_wait(allboarded);       //Wait until all skiers leave the bus
            writeFile(FINLEAV_BUS, -1, -1);

            //There is no more waiting skiers
            //OR there is for error, so let all the child processes to end
            if(*skiers_left == 0 || (*fork_err == 1 && *forked_skiers == 0)){ 
                writeFile(FINISH_BUS, -1, -1);
                sem_post(busfinish);    //Main process can continue now
                exit(0);
            }
        }

        //***************REGULAR STOP LEAVING***************

        else writeFile(LEAVING_BUS,-1,bus_stop);

    }
}

/**
 * @brief PARSE AND CHECK ARGUMENTS
 * 
 * @param argumentc Amount of arguments
 * @param argumentv Array of arguments
*/
void parseArguments(int argumentc, char**argumentv){
    if(argumentc != 6){
        fprintf(stderr, "There must be 5 arguments\n./proj2 L Z K TL TB\n");
        exit(1);
    }
    L = atoi(argumentv[1]);
    Z = atoi(argumentv[2]);   
    K = atoi(argumentv[3]);      
    TL = atoi(argumentv[4]);    
    TB = atoi(argumentv[5]);

    if(L<=0 || L>=20000) {
        fprintf(stderr, "Bad argument : 0<L<20000\n");
        exit(1);
    } //0<L<20000
    if(Z<=0 || Z>10) {
        fprintf(stderr, "Bad argument : 0<Z<=10\n");
        exit(1);
    } //0<Z<=10
    if(K<10 || K>100) {
        fprintf(stderr, "Bad arguments : 10<=K<=100\n");
        exit(1);
    }  //10<=K<=100
    if(TL<0 || TL>10000){
        fprintf(stderr, "Bad argument : 0<=TL<=10000\n");
        exit(1);
    } //0<=TL<=10000
    if(TB<0 || TB>1000) {
        fprintf(stderr, "Bad arguments : 0<=TB<=1000\n");
        exit(1);
    }//0<=TB<=1000
}

/**
 * @brief MAIN FUNCTION
*/
int main(int argc, char **argv){
    parseArguments(argc, argv); 

    if((file = fopen("proj2.out", "w")) == NULL){ 
        fprintf(stderr, "Failed to open file proj2.out\n");
        return 1;
    }
    
    initMemory();
    initSems();
    
    //Definition of shared memory
    *skiers_left=L;
    *current_capacity=0;
    *output_line = 0;
    for(int i = 0; i<Z;i++){
        stops_skiers[i] = 0;
    }
    *forked_skiers = 0;
    *fork_err = 0;

    //Skibus fork
    int bp = fork();
    if(bp == 0){
        skibusProcess();
    }
    else if(bp==-1){
        fprintf(stderr, "Failed to fork bus process\n");
        fclose(file);
        destSems();
        destMemory();
        return 1;
    }
    
    //Skiers fork
    for(int i = 0; i<L;i++){
        pid_t pid = fork();
        if(pid==0) {
            srand(time(0) * getpid() );
            skierProcess(i+1, (rand() %Z) + 1);
        }
        else if(pid == -1){
            fprintf(stderr, "Failed to fork skier process\n");
            *fork_err = 1;
            break;
        }
    }

    //Main process waiting for bus to finish
    sem_wait(busfinish);

    fclose(file);

    if(*fork_err == 1) {
        destSems();
        destMemory();   
        return 1;
    }
    
    destSems();
    destMemory();

    return 0;
}

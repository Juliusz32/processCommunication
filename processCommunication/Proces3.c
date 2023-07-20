#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <string.h>

int suspend;
int run;

static struct sembuf buf;

void hadlerWstrzymanieWyslanie(int sig){
    printf("Proces 3 (PID: %d) - wyslano sygnal wstrzymania\n", getpid());
    signal(sig, SIG_IGN);
    kill(getppid(), SIGILL);
    kill((getpid()-1), SIGILL);
    kill((getpid()-2), SIGILL);
}

void handlerWstrzymanie(int sig){
    printf("Proces 3 (PID: %d) - otrzymano sygnal wstrzymania\n", getpid());
    signal(sig, SIG_IGN);
    suspend = 1;
}

void handlerWznowienieWyslanie(int sig){
    printf("Proces 3 (PID: %d) - wyslano sygnal wznowienia\n", getpid());
    signal(sig, SIG_IGN);
    kill(getppid(), SIGTRAP);
    kill((getpid()-1), SIGTRAP);
    kill((getpid()-2), SIGTRAP);
}

void handlerWzowienie(int sig){
    printf("Proces 3 (PID: %d) - otrzymano sygnal wznowienia\n", getpid());
    signal(sig, SIG_IGN);
    suspend = 0;
}

void handlerZakonczenieWyslanie(int sig){
    printf("Proces 3 (PID: %d) - wyslano sygnal zakonczenia\n", getpid());
    signal(sig, SIG_IGN);
    kill(getppid(), SIGABRT);
    kill((getpid()-1), SIGABRT);
    kill((getpid()-2), SIGABRT);
    
}

void handlerZakonczenie(int sig){
    printf("Proces 3 (PID: %d) - otrzymano sygnal zakonczenia\n", getpid());
    signal(sig, SIG_IGN);
    run = 0;
}

void sig(int semid, int semnum){
    buf.sem_num = semnum;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1){
        perror("Podnoszenie semafora");
    }
}

void wait(int semid, int semnum){
    buf.sem_num = semnum;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1){
        perror("Opuszczenie semafora");
    }
}

int main(void) {

    printf("Proces 3 (PID: %d)\n", getpid());

    //pamiec
    int shmid, i, semid;
    char *buf;

    semid = semget(45287, 2, IPC_CREAT|0600);
    if (semid == -1){
        perror("blad tworzenia tablicy semaforow");
        exit(1);
    }

    shmid = shmget(45286, sizeof(char), IPC_CREAT|0600);
    if (shmid == -1){
        perror("blad tworzenia segmentu pamieci wspoldzielonej");
        exit(1);
    }

    buf = shmat(shmid, NULL, 0);
    if (buf == NULL){
        perror("blad przylaczenia segmentu pamieci wspoldzielonej");
        exit(1);
    }

    suspend = 0;

    while(1){
        signal(SIGHUP, hadlerWstrzymanieWyslanie);
        signal(SIGILL, handlerWstrzymanie);
        signal(SIGINT, handlerWznowienieWyslanie);
        signal(SIGTRAP, handlerWzowienie);
        signal(SIGQUIT, handlerZakonczenieWyslanie);
        signal(SIGABRT, handlerZakonczenie);

        

        //printf("1sus: %i\n", suspend);
        run = 1;

        if(suspend == 0){
            wait(semid, 1);
            if((suspend == 0) && (run = 1)){
                printf("P3 (PID: %d) odczytano z pamieci: %s\n", getpid(), buf);
                sig(semid, 0);
            }
            
        }

        
        


    }

    sleep(1);

    return 0;
}
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

#define FIFO "my_fifo"

/*
sygnaly:

(1) SIGHUP       -  Wstrzymanie - sygnał zawieszenia
(2) SIGINT       -  Wznowienie wstrzymanego procesu   
(3) SIGSIGQUIT   -  Zatrzymanie procesow

*/

int pid1;
int pid2;
int pid3;
int run;

static struct sembuf buf;

void sig(int semid, int semnum){
    buf.sem_num = semnum;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1){
        perror("Podnoszenie semafora");
        exit(1);
    }
}

void wait(int semid, int semnum){
    buf.sem_num = semnum;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1){
        perror("Opuszczenie semafora");
        exit(1);
    }
}

void hadlerWstrzymanieWyslanie(int sig){
    printf("Proces Macierzysty (PID: %d) - wyslano sygnal wstrzymania\n", getpid());
    signal(sig, SIG_IGN);
    kill(pid1, SIGILL); //4
    kill(pid2, SIGILL);
    kill(pid3, SIGILL);
    kill(getpid(), SIGILL);
}

void handlerWstrzymanie(int sig){
    printf("Proces Macierzysty (PID: %d) - otrzymano sygnal wstrzymania\n", getpid());
    signal(sig, SIG_IGN);
    kill(getpid(), SIGHUP);
}

void handlerWznowienieWyslanie(int sig){
    printf("Proces Macierzysty (PID: %d) - wyslano sygnal wznowienia\n", getpid());
    signal(sig, SIG_IGN);
    kill(pid1, SIGTRAP);
    kill(pid2, SIGTRAP);
    kill(pid3, SIGTRAP);
    kill(getpid(), SIGTRAP);
}

void handlerWzowienie(int sig){
    printf("Proces Macierzysty (PID: %d) - otrzymano sygnal wstrzymania\n", getpid());
    signal(sig, SIG_IGN);
    kill(getpid(), SIGINT);
}

void handlerZakonczenieWyslanie(int sig){
    printf("Proces Macierzysty (PID: %d) - wyslano sygnal zakonczenia\n", getpid());
    signal(sig, SIG_IGN);
    kill(pid1, SIGABRT);
    kill(pid2, SIGABRT);
    kill(pid3, SIGABRT);
    kill(getpid(), SIGABRT);
}

void handlerZakonczenie(int sig){
    printf("Proces Macierzysty (PID: %d) - otrzymano sygnal zakonczenia\n", getpid());
    signal(sig, SIG_IGN);
    kill(getpid(), SIGQUIT);
    run = 0;
    sleep(2);
    kill(pid1, 9);
    kill(pid2, 9);
    kill(pid3, 9);
}




int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Bledne parametry!\n");
        return 0;
    }

    int shmid, semid;
    char *buf;
    

    semid = semget(45287, 2, IPC_CREAT|0600);
    if (semid == -1){
        perror("blad tworzenia tablicy semaforow");
        exit(1);
    }

    //semafory
    if (semctl(semid, 0, SETVAL, (int)1) == -1){
        perror("blad nadawania wartosci semaforowi 0");
        exit(1);
    }
    if (semctl(semid, 1, SETVAL, (int)0) == -1){
        perror("blad nadawania wartosci semaforowi 1");
        exit(1);
    }

    //pamiec

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

    //maska

    sigset_t maskaM;
    sigfillset(&maskaM);
    sigdelset(&maskaM, SIGHUP);
    sigdelset(&maskaM, SIGILL);
    sigdelset(&maskaM, SIGINT);
    sigdelset(&maskaM, SIGTRAP);
    sigdelset(&maskaM, SIGQUIT);
    sigdelset(&maskaM, SIGABRT);
    sigprocmask(SIG_SETMASK, &maskaM, NULL);

    printf("Proces macierzysty (PID: %d)\n", getpid());

    if((pid1 = fork()) == 0){
        execlp("./Proces1", "./Proces1", argv[1], NULL);
    }else if((pid2 = fork()) == 0){
        execlp("./Proces2", "./Proces2", argv[1], NULL);
    }else if((pid3 = fork()) == 0){
        execlp("./Proces3", "./Proces3", (char*)NULL);
    }else{
        run = 1;
        while(run == 1){
            
            signal(SIGHUP, hadlerWstrzymanieWyslanie);
            signal(SIGILL, handlerWstrzymanie);
            signal(SIGINT, handlerWznowienieWyslanie);
            signal(SIGTRAP, handlerWzowienie);
            signal(SIGQUIT, handlerZakonczenieWyslanie);
            signal(SIGABRT, handlerZakonczenie);
            sleep(2);
        }
    }


    shmdt(buf);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    semctl(semid, 1, IPC_RMID);
    unlink(FIFO);



    return 0;

}
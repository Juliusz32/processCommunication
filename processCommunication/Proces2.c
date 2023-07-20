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
static struct sembuf buf;

int suspend;

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

void hadlerWstrzymanieWyslanie(int sig){
    printf("Proces 2 (PID: %d) - wyslano sygnal wstrzymania\n", getpid());
    signal(sig, SIG_IGN);
    kill(getppid(), SIGILL);
    kill((getpid()-1), SIGILL);
    kill((getpid()+1), SIGILL);
}

void handlerWstrzymanie(int sig){
    printf("Proces 2 (PID: %d) - otrzymano sygnal wstrzymania\n", getpid());
    signal(sig, SIG_IGN);
    suspend = 1;
}

void handlerWznowienieWyslanie(int sig){
    printf("Proces 2 (PID: %d) - wyslano sygnal wznowienia\n", getpid());
    signal(sig, SIG_IGN);
    kill(getppid(), SIGTRAP);
    kill((getpid()-1), SIGTRAP);
    kill((getpid()+1), SIGTRAP);
}

void handlerWzowienie(int sig){
    printf("Proces 2 (PID: %d) - otrzymano sygnal wznowienia\n", getpid());
    signal(sig, SIG_IGN);
    suspend = 0;
}

void handlerZakonczenieWyslanie(int sig){
    printf("Proces 2 (PID: %d) - wyslano sygnal zakonczenia\n", getpid());
    signal(sig, SIG_IGN);
    kill(getppid(), SIGABRT);
    kill((getpid()+1), SIGABRT);
    kill((getpid()-1), SIGABRT);
    
}

void handlerZakonczenie(int sig){
    printf("Proces 2 (PID: %d) - otrzymano sygnal zakonczenia\n", getpid());
    signal(sig, SIG_IGN);
}



int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Proces 1: Bledne parametry!\n");
        return 0;
    }

    printf("Proces 2 (PID: %d)\n", getpid());

    //tworzenie kolejki
    mkfifo(FIFO, 0666);
    char in[1], out[1], napis[128], source[128], temp1[200];
    char chr;
    int fifo;
    int len = 0;
    int sync = 0;
    char character[4];

    int znak = 0;
    int i = 0;

    FILE *outputFile;
        

    sprintf(temp1, "%s_mod1", argv[1]);

    //tworzenie pamieci wspoldzielonej i semaforow
    int shmid, semid;
    char *buf;
    

    semid = semget(45287, 2, IPC_CREAT|0600);
    if (semid == -1){
        perror("blad tworzenia tablicy semaforow");
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
    
    suspend = 0;

    if((outputFile = fopen(temp1, "w+")) == NULL){
                fprintf(stderr, "Blad tworzenia pliku!\n");
                return 0;
            }

    while(sync != 2){

        signal(SIGHUP, hadlerWstrzymanieWyslanie);
        signal(SIGILL, handlerWstrzymanie);
        signal(SIGINT, handlerWznowienieWyslanie);
        signal(SIGTRAP, handlerWzowienie);
        signal(SIGQUIT, handlerZakonczenieWyslanie);
        signal(SIGABRT, handlerZakonczenie);

        if(suspend == 0){

        fifo = open(FIFO, O_RDONLY);


        while(read(fifo,out,1)>0 && *out != '\n'){
            
            printf("P2 (PID: %d) odczytano: %c\tpo konwersji: 0x%x\n", getpid(), *out, *out);
            fprintf(outputFile, "0x%x\n", *out);

            sync = 0;
            znak++;

            wait(semid, 0);
            sprintf(character, "0x%x", *out);
            buf[0] = character[0];
            buf[1] = character[1];
            buf[2] = character[2];
            buf[3] = character[3];
            printf("P2 (PID: %d) zapisuje do pamieci: %s\n", getpid(), buf);
            sig(semid, 1);

        }
        if(*out = '\n'){
            sync++;
        }

        close(fifo);

        }

        signal(SIGHUP, hadlerWstrzymanieWyslanie);
        signal(SIGILL, handlerWstrzymanie);
        signal(SIGINT, handlerWznowienieWyslanie);
        signal(SIGTRAP, handlerWzowienie);
        signal(SIGQUIT, handlerZakonczenieWyslanie);
        signal(SIGABRT, handlerZakonczenie);

        
        
    }
    fclose(outputFile);

    

    

    


    printf("Koniec dzialania P2 (PID: %d), pobrano %i znakow\n", getpid(), znak);

    kill(getppid(), SIGABRT);

    return 0;

}
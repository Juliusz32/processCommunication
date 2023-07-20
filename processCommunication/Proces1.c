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
int pid;

int suspend;
int run;

void hadlerWstrzymanieWyslanie(int sig){
    printf("Proces 1 (PID: %d) - wyslano sygnal wstrzymania\n", getpid());
    signal(sig, SIG_IGN);
    kill(getppid(), SIGILL);
    kill((getpid()+1), SIGILL);
    kill((getpid()+2), SIGILL);
}

void handlerWstrzymanie(int sig){
    printf("Proces 1 (PID: %d) - otrzymano sygnal wstrzymania\n", getpid());
    signal(sig, SIG_IGN);
    suspend = 1;
}

void handlerWznowienieWyslanie(int sig){
    printf("Proces 1 (PID: %d) - wyslano sygnal wznowienia\n", getpid());
    signal(sig, SIG_IGN);
    kill(getppid(), SIGTRAP);
    kill((getpid()+1), SIGTRAP);
    kill((getpid()+2), SIGTRAP);
}

void handlerWzowienie(int sig){
    printf("Proces 1 (PID: %d) - otrzymano sygnal wznowienia\n", getpid());
    signal(sig, SIG_IGN);
    suspend = 0;
}

void handlerZakonczenieWyslanie(int sig){
    printf("Proces 1 (PID: %d) - wyslano sygnal zakonczenia\n", getpid());
    signal(sig, SIG_IGN);
    kill(getppid(), SIGABRT);
    kill((getpid()+1), SIGABRT);
    kill((getpid()+2), SIGABRT);
    
}

void handlerZakonczenie(int sig){
    printf("Proces 1 (PID: %d) - otrzymano sygnal zakonczenia\n", getpid());
    signal(sig, SIG_IGN);
    unlink(FIFO);
}



int main(int argc, char *argv[]) {
    int choice;


    if (argc != 2) {
        fprintf(stderr, "Proces 1: Bledne parametry!\n");
        return 0;
    }

    int pid = getpid();

    printf("Proces 1 (PID: %d)\n", pid);

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



    //tworzenie kolejki
    mkfifo(FIFO, 0666);
    char in[1], out[1], napis[128], source[128], temp1[200];
    char chr;
    int fifo;
    int len = 0;

    //pliki
    FILE *inputFile;

    printf("Wybierz zrodlo danych:\n1-plik\n2-klawiatura\n");
    scanf("%d", &choice);
    if(choice == 1){
        if((inputFile = fopen(argv[1], "r")) == NULL){
            fprintf(stderr, "Blad odczytu pliku!\n");
            return 0;
        }


        while((chr=fgetc(inputFile))!=EOF){
            napis[len] = chr;
            len++;
        }

        fclose(inputFile);

        int j = 0;
        while(j < len){  

            signal(SIGHUP, hadlerWstrzymanieWyslanie);
            signal(SIGILL, handlerWstrzymanie);
            signal(SIGINT, handlerWznowienieWyslanie);
            signal(SIGTRAP, handlerWzowienie);
            signal(SIGQUIT, handlerZakonczenieWyslanie);
            signal(SIGABRT, handlerZakonczenie);

            fifo = open(FIFO, O_WRONLY);
            *in = *(napis+j);
            write(fifo, in, 1);
            if(*in != '\n'){
                printf("P1 (PID: %d) zapisano: %c\n", getpid(), *in);
            }
            close(fifo);
            j++;

        }
        unlink(FIFO);
    }if(choice == 2){
        
        int end = 1;
        printf("Podaj dane z klawiatury\n");
        suspend = 0;
    
        while(end!=3){
            
            signal(SIGHUP, hadlerWstrzymanieWyslanie);
            signal(SIGILL, handlerWstrzymanie);
            signal(SIGINT, handlerWznowienieWyslanie);
            signal(SIGTRAP, handlerWzowienie);
            signal(SIGQUIT, handlerZakonczenieWyslanie);
            signal(SIGABRT, handlerZakonczenie);

            fifo = open(FIFO, O_WRONLY);
            
            scanf("%c", in);

            signal(SIGHUP, hadlerWstrzymanieWyslanie);
            signal(SIGILL, handlerWstrzymanie);
            signal(SIGINT, handlerWznowienieWyslanie);
            signal(SIGTRAP, handlerWzowienie);
            signal(SIGQUIT, handlerZakonczenieWyslanie);
            signal(SIGABRT, handlerZakonczenie);

            while(suspend == 1){
                printf("Komunikacja wstrzymana!\n");
                sleep(1);
            }


            if(*in == '\n'){
                end++;
            }else{
                end = 0;
            }
            write(fifo, in, 1);
            if(*in != '\n'){
                printf("P1 (PID: %d) zapisano: %c\n", getpid(), *in);
            }

            
            close(fifo);

            

        }
        unlink(FIFO);
    }


    

    return 0;
}
#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Bledne parametry!\n");
        return 0;
    }

    int num  = 0;
    char str[100], temp1[200];
    char znak;
    char chr;

    FILE *inputFile;

    sprintf(temp1, "%s_mod1", argv[1]);
    if((inputFile = fopen(temp1, "r")) == NULL){
        fprintf(stderr, "Blad odczytu pliku!\n");
        return 0;
    }

    while(fgets(str, sizeof(str), inputFile)){
        num = strtol(str, 0, 0);
        znak = num;
        printf("%c", znak);
    }

    printf("\n");


    fclose(inputFile);

    return 0;
}


#include<stdio.h>
#include<stdlib.h>
#include<string.h>

    typedef enum{ fácil = 0, médio, difícil, muitodifícil} dificuldade;

    typedef struct 
    {
     char enunciado[200];
    char alternativas[4][100];
     char correta[1];
     dificuldade nivel;
    }Pergunta;
    
    int main()
    {
       int capaPERGNTA = 15. tamPERGNTA = 0;
            Pergunta * Cadastro (Pergunta*) malloc(capaPERGNTA* sizeof(Pergunta));
            if (Cadastro == NULL)
            {
                printf("faltou memoria");
                    exit(EXIT_FAILURE);
            }
            
                if (tamPERGNTA == capaPERGNTA )
                {
                    capaPERGNTA += 2;
                }
                
        return 0;
    }
    
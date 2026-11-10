
#include <stdio.h>
#include <stdlib.h>
#include "flameChaos.h"

int main(void)
{

    int capacidade = 5;
    int tamanho = 0;

    Pergunta *listaPerguntas = malloc(capacidade * sizeof(Pergunta));

    if (listaPerguntas == NULL)
    {
        printf("Erro: Falha ao alocar memoria!\n");
        return EXIT_FAILURE;
    }

    int opcao;

    do
    {
        printf("\n");
        printf("================================\n");
        printf("        FLAME CHAOS\n");
        printf("================================\n");
        printf("  1. Cadastrar Pergunta\n");
        printf("  2. Listar Perguntas\n");
        printf("  3. Pesquisar Pergunta\n");
        printf("  4. Alterar Pergunta\n");
        printf("  5. Excluir Pergunta\n");
        printf("  0. Sair\n");
        printf("================================\n");
        printf("Escolha uma opcao: ");

        if (scanf("%d", &opcao) != 1)
        {
            printf("Entrada invalida!\n");

            int caractere;
            while ((caractere = getchar()) != '\n' &&
                   caractere != EOF)
                ;

            continue;
        }

        switch (opcao)
        {

        case 1:
            cadastrarPergunta(
                &listaPerguntas,
                &tamanho,
                &capacidade);
            break;

        case 2:
            listarPerguntas(
                listaPerguntas,
                tamanho);
            break;

        case 3:
            pesquisarPergunta(
                listaPerguntas,
                tamanho);
            break;

        case 4:
            alterarPergunta(
                listaPerguntas,
                tamanho);
            break;

        case 5:
            excluirPergunta(
                &listaPerguntas,
                &tamanho,
                &capacidade);
            break;

        case 0:
            printf("\nEncerrando FlameChaos...\n");
            break;

        default:
            printf("\nOpcao invalida!\n");
            break;
        }

    } while (opcao != 0);

    free(listaPerguntas);

    return EXIT_SUCCESS;
}

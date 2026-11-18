//main de teste(n [e a main oficial apenas um jeito de testar para ver se os arquivos est;ao corretos])
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "pergunta.h"
#include "arquivo.h"
#include "jogo.h"

int main() {

    srand((unsigned int)time(NULL));

    Pergunta *perguntas = NULL;
    int quantidade = 0;

    // Carrega as perguntas
    if (!carregarDados(&perguntas, &quantidade)) {
        printf("Erro ao carregar perguntas!\n");
        return 1;
    }

    printf("Total de perguntas carregadas: %d\n", quantidade);

    // Inicia uma partida
    jogarPartida(perguntas, quantidade);

    // Salva os dados ao encerrar
    if (!salvarBinario(perguntas, quantidade)) {
        printf("Erro ao salvar perguntas!\n");
    }

    free(perguntas);

    return 0;
}

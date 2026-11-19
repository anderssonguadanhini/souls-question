/**
 * @file main.c
 * @brief Programa principal: carrega as perguntas, mostra o menu e
 *        salva tudo em arquivo binário ao sair.
 */

#ifdef _WIN32
/* O windows.h DEVE vir antes do raylib.h.
 * 1) Funcoes com o mesmo nome nos dois: renomeadas so durante o include. */
#define Rectangle   Win32Rectangle
#define CloseWindow Win32CloseWindow
#define ShowCursor  Win32ShowCursor
#include <windows.h>
#undef Rectangle
#undef CloseWindow
#undef ShowCursor
/* 2) Macros do windows.h que trocam o nome (ex.: LoadImage -> LoadImageA)
 *    e quebrariam as funcoes de mesmo nome do raylib. */
#undef DrawText
#undef DrawTextEx
#undef LoadImage
#undef PlaySound

/* MinGW antigo nao define esta constante */
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include "raylib.h" /* reservado para a interface gráfica (parte criativa) */
#include "flameChaos.h"
#include "jogo_demilhao.h"

/**
 * @brief Mostra o menu principal.
 */
static void mostrarMenu(void)
{
    printf("\n");
    printf("%s================================%s\n", COR_CIANO, COR_RESET);
    printf("%s        FLAME CHAOS%s\n", COR_BRANCO_NEGRITO, COR_RESET);
    printf("%s================================%s\n", COR_CIANO, COR_RESET);
    printf("  1. Cadastrar Pergunta\n");
    printf("  2. Listar Perguntas\n");
    printf("  3. Pesquisar por Nível\n");
    printf("  4. Alterar Pergunta\n");
    printf("  5. Excluir Pergunta\n");
    printf("  6. Exportar para CSV\n");
    printf("  0. Sair\n");
    printf("%s================================%s\n", COR_CIANO, COR_RESET);
}

/**
 * @brief Função principal.
 * @param argc Quantidade de argumentos.
 * @param argv Argumentos: o primeiro (opcional) é o caminho do CSV.
 *             Ex.: ./flameChaos caminho/do/perguntas.csv
 * @return EXIT_SUCCESS se tudo deu certo, EXIT_FAILURE se não carregou as perguntas.
 */
int main(int argc, char *argv[])
{
#ifdef _WIN32
    // Liga as cores ANSI no console do Windows
    HANDLE saida = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD modo = 0;
    GetConsoleMode(saida, &modo);
    SetConsoleMode(saida, modo | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

    configurarTerminal(); // UTF-8 no Windows (no Linux não faz nada)

    Banco banco = {NULL, 0, 0};              // vetor dinâmico vazio
    char caminhoBin[TAM_CAMINHO] = NOME_BIN; // onde o .bin será gravado

    // Se o usuário passou um caminho ao executar, usa ele como CSV
    const char *csvInformado = (argc > 1) ? argv[1] : NULL;

    // 1ª execução: lê o CSV. Nas outras: lê o binário
    if (iniciarBanco(&banco, csvInformado, caminhoBin, sizeof(caminhoBin)) <= 0)
    {
        printf("Erro ao carregar as perguntas.\n");
        printf("Confira se o CSV existe, está em UTF-8, separado por vírgula e com o formato certo.\n");
        printf("Você também pode executar assim: %s caminho/do/arquivo.csv\n", argv[0]);
        free(banco.dados);
        return EXIT_FAILURE;
    }
    printf("Perguntas carregadas: %d\n", banco.quantidade);

    int opcao = -1;
    do
    {
        mostrarMenu();

        int leitura = lerInteiro("Escolha uma opção: ", &opcao);
        if (leitura == -1)
        {
            opcao = 0; // entrada fechada: sai salvando
        }
        else if (leitura == 0)
        {
            tocarSom(3);
            printf("%sEntrada inválida!%s\n", COR_VERMELHO, COR_RESET);
            opcao = -1;
            continue;
        }

        switch (opcao)
        {
        case 1:
            cadastrarPergunta(&banco);
            break;
        case 2:
            listarPerguntas(&banco);
            break;
        case 3:
            pesquisarPergunta(&banco);
            break;
        case 4:
            alterarPergunta(&banco);
            break;
        case 5:
            excluirPergunta(&banco);
            break;
        case 6:
            exportarPerguntas(&banco);
            break;
        case 0:
            printf("\nSalvando dados...\n");
            if (salvarBinario(&banco, caminhoBin) == 0)
            {
                printf("%s[OK] %d pergunta(s) salva(s) em '%s'.%s\n",
                       COR_VERDE, banco.quantidade, caminhoBin, COR_RESET);
            }
            else
            {
                tocarSom(3);
                printf("%s[!] Erro ao salvar '%s'!%s\n", COR_VERMELHO, caminhoBin, COR_RESET);
            }
            printf("Encerrando FlameChaos...\n");
            break;
        default:
            tocarSom(3);
            printf("\n%sOpção inválida!%s\n", COR_VERMELHO, COR_RESET);
            break;
        }
    } while (opcao != 0);

    free(banco.dados); // devolve a memória do vetor dinâmico
    return EXIT_SUCCESS;
}

/**
 * @file flameChaos.c
 * @brief Implementação do menu e do cadastro de perguntas.
 *
 * Todas as funções trabalham sobre o Banco (vetor dinâmico) lido do
 * CSV/binário por jogo_demilhao.c. Nenhuma variável global é usada.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "flameChaos.h"

#ifdef _WIN32
#include <windows.h> /* Beep */
#endif

/** Capacidade inicial do vetor e tamanho mínimo para ele encolher. */
#define CAPACIDADE_INICIAL 16

/* ---------------------------------------------------------------
 * Funções auxiliares (visíveis só neste arquivo)
 * --------------------------------------------------------------- */

/** @brief Lê uma linha do teclado sem o Enter. Retorna 0 se der EOF. */
static int lerLinha(const char *mensagem, char *destino, size_t tam)
{
    printf("%s", mensagem);
    fflush(stdout);

    if (fgets(destino, (int)tam, stdin) == NULL)
    {
        return 0;
    }

    size_t tamanhoLido = strcspn(destino, "\r\n");
    if (destino[tamanhoLido] == '\0' && tamanhoLido == tam - 1)
    {
        // a linha era maior que o espaço disponível: descarta o resto
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
        {
        }
    }
    destino[tamanhoLido] = '\0';
    return 1;
}

/** @brief Lê um texto não vazio e sem vírgula (a vírgula quebraria o CSV). */
static int lerTexto(const char *mensagem, char *destino, size_t tam)
{
    while (1)
    {
        if (!lerLinha(mensagem, destino, tam))
        {
            return 0;
        }
        if (destino[0] == '\0')
        {
            tocarSom(3);
            printf("%s[!] O campo não pode ficar vazio.%s\n", COR_VERMELHO, COR_RESET);
        }
        else if (strchr(destino, ',') != NULL)
        {
            tocarSom(3);
            printf("%s[!] Não use vírgula (ela separa as colunas do CSV).%s\n",
                   COR_VERMELHO, COR_RESET);
        }
        else
        {
            return 1;
        }
    }
}

/** @brief Lê a letra da alternativa correta (A a D). Retorna 0 se der EOF. */
static int lerLetra(const char *mensagem, char *letra)
{
    char linha[16];
    while (1)
    {
        if (!lerLinha(mensagem, linha, sizeof(linha)))
        {
            return 0;
        }
        char c = (char)toupper((unsigned char)linha[0]);
        if (linha[0] != '\0' && linha[1] == '\0' && c >= 'A' && c <= 'D')
        {
            *letra = c;
            return 1;
        }
        tocarSom(3);
        printf("%s[!] Digite apenas A, B, C ou D.%s\n", COR_VERMELHO, COR_RESET);
    }
}

/** @brief Lê um nível de 1 a 5. Retorna 0 se der EOF. */
static int lerNivel(const char *mensagem, int *nivel)
{
    while (1)
    {
        int r = lerInteiro(mensagem, nivel);
        if (r == -1)
        {
            return 0;
        }
        if (r == 1 && *nivel >= NIVEL_MIN && *nivel <= NIVEL_MAX)
        {
            return 1;
        }
        tocarSom(3);
        printf("%s[!] Digite um número de %d a %d.%s\n", COR_VERMELHO, NIVEL_MIN, NIVEL_MAX, COR_RESET);
    }
}

/** @brief Calcula o próximo ID livre (maior ID existente + 1). */
static int proximoId(const Banco *banco)
{
    int maior = 0;
    for (int i = 0; i < banco->quantidade; i++)
    {
        if (banco->dados[i].id > maior)
        {
            maior = banco->dados[i].id;
        }
    }
    return maior + 1;
}

/** @brief Pede todos os campos de uma pergunta. Retorna 0 se der EOF. */
static int preencherPergunta(Perguntas *p)
{
    static const char letras[4] = {'A', 'B', 'C', 'D'};
    char mensagem[40];

    if (!lerTexto("Jogo de origem: ", p->jogo, sizeof(p->jogo)) ||
        !lerTexto("Enunciado: ", p->enunciado, sizeof(p->enunciado)))
    {
        return 0;
    }
    for (int i = 0; i < 4; i++)
    {
        snprintf(mensagem, sizeof(mensagem), "Alternativa [%c]: ", letras[i]);
        if (!lerTexto(mensagem, p->alternativa[i], sizeof(p->alternativa[i])))
        {
            return 0;
        }
    }
    if (!lerLetra("Alternativa correta (A, B, C ou D): ", &p->correta))
    {
        return 0;
    }
    return lerNivel("Nível (1-Muito Fácil, 2-Fácil, 3-Médio, 4-Difícil, 5-Muito Difícil): ", &p->nivel);
}

/** @brief Imprime o enunciado e as alternativas de uma pergunta. */
static void imprimirPergunta(const Perguntas *p, int destacarCorreta)
{
    static const char letras[4] = {'A', 'B', 'C', 'D'};
    printf("  %s\n\n", p->enunciado);
    for (int i = 0; i < 4; i++)
    {
        if (destacarCorreta && p->correta == letras[i])
        {
            printf("  %s--> [%c] %s  (CORRETA!)%s\n", COR_VERDE, letras[i], p->alternativa[i], COR_RESET);
        }
        else
        {
            printf("      [%c] %s\n", letras[i], p->alternativa[i]);
        }
    }
}

/** @brief Mostra quantas perguntas existem por nível e o mínimo exigido. */
static void mostrarResumoNiveis(const Banco *banco)
{
    // mínimo de perguntas por nível exigido no enunciado do projeto
    static const int minimo[NIVEL_MAX + 1] = {0, 20, 20, 15, 15, 10};
    int contagem[NIVEL_MAX + 1] = {0};

    for (int i = 0; i < banco->quantidade; i++)
    {
        int n = banco->dados[i].nivel;
        if (n >= NIVEL_MIN && n <= NIVEL_MAX)
        {
            contagem[n]++;
        }
    }

    printf("\n%sResumo por nível (atual / mínimo):%s\n", COR_BRANCO_NEGRITO, COR_RESET);
    for (int n = NIVEL_MIN; n <= NIVEL_MAX; n++)
    {
        printf("  %s%-14s%s %3d / %-3d", obterCorNivel(n), textoDoNivel(n), COR_RESET,
               contagem[n], minimo[n]);
        if (contagem[n] < minimo[n])
        {
            printf("  %s(faltam %d)%s", COR_AMARELO, minimo[n] - contagem[n], COR_RESET);
        }
        printf("\n");
    }
}

/* ---------------------------------------------------------------
 * Funções públicas
 * --------------------------------------------------------------- */

void tocarSom(int tipo)
{
#ifdef _WIN32
    switch (tipo)
    {
    case 1:
        Beep(750, 90);
        break;
    case 2:
        Beep(523, 100);
        Beep(659, 100);
        Beep(784, 150);
        break;
    case 3:
        Beep(250, 200);
        break;
    case 4:
        Beep(440, 120);
        Beep(415, 120);
        Beep(392, 250);
        break;
    }
#else
    (void)tipo;
    printf("\a");
    fflush(stdout);
#endif
}

const char *obterCorNivel(int nivel)
{
    switch (nivel)
    {
    case 1:
        return COR_VERDE;
    case 2:
        return COR_CIANO;
    case 3:
        return COR_AMARELO;
    case 4:
        return COR_VERMELHO;
    case 5:
        return COR_ROXO;
    default:
        return COR_RESET;
    }
}

int lerInteiro(const char *mensagem, int *valor)
{
    char linha[64];
    if (!lerLinha(mensagem, linha, sizeof(linha)))
    {
        return -1;
    }

    char *fim;
    long numero = strtol(linha, &fim, 10);
    if (fim == linha)
    {
        return 0; // não começou com número
    }
    while (isspace((unsigned char)*fim))
    {
        fim++;
    }
    if (*fim != '\0')
    {
        return 0; // sobrou texto depois do número
    }

    *valor = (int)numero;
    return 1;
}

void cadastrarPergunta(Banco *banco)
{
    // Se o vetor está cheio, aumenta a capacidade (dobra)
    if (banco->quantidade >= banco->capacidade)
    {
        int novaCapacidade = (banco->capacidade == 0) ? CAPACIDADE_INICIAL : banco->capacidade * 2;
        Perguntas *temp = realloc(banco->dados, novaCapacidade * sizeof(Perguntas));
        if (temp == NULL)
        {
            tocarSom(3);
            printf("%s[!] Erro ao realocar memória!%s\n", COR_VERMELHO, COR_RESET);
            return;
        }
        banco->dados = temp;
        banco->capacidade = novaCapacidade;
    }

    printf("\n--- CADASTRAR NOVA PERGUNTA ---\n");
    Perguntas nova;
    nova.id = proximoId(banco);
    if (!preencherPergunta(&nova))
    {
        return; // entrada fechada: não cadastra pela metade
    }

    banco->dados[banco->quantidade] = nova;
    banco->quantidade++;
    tocarSom(2);
    printf("%s[OK] Pergunta #%d cadastrada com sucesso!%s\n", COR_VERDE, nova.id, COR_RESET);
}

void listarPerguntas(const Banco *banco)
{
    if (banco->quantidade == 0)
    {
        tocarSom(3);
        printf("\n%s[!] Nenhuma pergunta cadastrada.%s\n", COR_AMARELO, COR_RESET);
        return;
    }

    static const char letras[4] = {'A', 'B', 'C', 'D'};
    printf("\n================ LISTA DE PERGUNTAS (%d) ================\n", banco->quantidade);
    for (int i = 0; i < banco->quantidade; i++)
    {
        const Perguntas *p = &banco->dados[i];
        printf("\n%s[#%d] [%s] (%s)%s %s\n", obterCorNivel(p->nivel), p->id,
               textoDoNivel(p->nivel), p->jogo, COR_RESET, p->enunciado);
        for (int j = 0; j < 4; j++)
        {
            printf("   [%c] %s\n", letras[j], p->alternativa[j]);
        }
        printf("   Correta: %s%c%s\n", COR_VERDE, p->correta, COR_RESET);
    }
    printf("=========================================================\n");
    mostrarResumoNiveis(banco);
}

void pesquisarPergunta(const Banco *banco)
{
    if (banco->quantidade == 0)
    {
        tocarSom(3);
        printf("\n%s[!] Nenhuma pergunta cadastrada no sistema.%s\n", COR_VERMELHO, COR_RESET);
        return;
    }

    limparTela();
    printf("%s==================================================%s\n", COR_CIANO, COR_RESET);
    printf("%s       PESQUISAR PERGUNTAS POR DIFICULDADE        %s\n", COR_BRANCO_NEGRITO, COR_RESET);
    printf("%s==================================================%s\n", COR_CIANO, COR_RESET);
    for (int n = NIVEL_MIN; n <= NIVEL_MAX; n++)
    {
        printf("  %s[%d] %s%s\n", obterCorNivel(n), n, textoDoNivel(n), COR_RESET);
    }
    printf("%s==================================================%s\n", COR_CIANO, COR_RESET);

    int nivelBuscado;
    int leitura = lerInteiro("Escolha o nível desejado: ", &nivelBuscado);
    if (leitura == -1)
    {
        return;
    }
    if (leitura != 1 || nivelBuscado < NIVEL_MIN || nivelBuscado > NIVEL_MAX)
    {
        tocarSom(3);
        printf("\n%s[!] Nível inválido!%s\n", COR_VERMELHO, COR_RESET);
        return;
    }

    // Guarda as posições (no vetor) das perguntas do nível escolhido
    int *indices = malloc(banco->quantidade * sizeof(int));
    if (indices == NULL)
    {
        tocarSom(3);
        printf("%s[!] Erro de memória!%s\n", COR_VERMELHO, COR_RESET);
        return;
    }
    int total = 0;
    for (int i = 0; i < banco->quantidade; i++)
    {
        if (banco->dados[i].nivel == nivelBuscado)
        {
            indices[total++] = i;
        }
    }

    if (total == 0)
    {
        tocarSom(3);
        printf("\n%s[!] Nenhuma pergunta encontrada para o nível %s.%s\n",
               COR_AMARELO, textoDoNivel(nivelBuscado), COR_RESET);
        free(indices);
        return;
    }

    tocarSom(nivelBuscado == NIVEL_MAX ? 4 : 1);

    const char *corTema = obterCorNivel(nivelBuscado);
    int posicao = 0;
    int mostrarGabarito = 0;
    char comando = ' ';

    do
    {
        const Perguntas *p = &banco->dados[indices[posicao]];

        limparTela();
        printf("%s+----------------------------------------------------------+%s\n", corTema, COR_RESET);
        printf("%s  FLAME CHAOS - NÍVEL: %s   [Card %d de %d]%s\n",
               corTema, textoDoNivel(nivelBuscado), posicao + 1, total, COR_RESET);
        printf("%s+----------------------------------------------------------+%s\n", corTema, COR_RESET);

        printf("  Progresso: [");
        for (int i = 0; i < total; i++)
        {
            if (i == posicao)
            {
                printf("%s#%s", corTema, COR_RESET);
            }
            else
            {
                printf("-");
            }
        }
        printf("] (ID: #%d | Jogo: %s)\n\n", p->id, p->jogo);

        printf("  %sPERGUNTA:%s\n", COR_BRANCO_NEGRITO, COR_RESET);
        imprimirPergunta(p, mostrarGabarito);

        printf("\n%s+----------------------------------------------------------+%s\n", corTema, COR_RESET);
        printf("  %s[A]%s Anterior | %s[D]%s Próxima | %s[R]%s Revelar | %s[S]%s Sair\n",
               COR_CIANO, COR_RESET, COR_CIANO, COR_RESET,
               COR_AMARELO, COR_RESET, COR_VERMELHO, COR_RESET);

        char linha[16];
        if (!lerLinha("Comando: ", linha, sizeof(linha)))
        {
            comando = 'S'; // entrada fechada: sai
        }
        else
        {
            comando = (char)toupper((unsigned char)linha[0]);
        }

        switch (comando)
        {
        case 'D':
            if (posicao < total - 1)
            {
                posicao++;
                mostrarGabarito = 0;
                tocarSom(1);
            }
            else
            {
                tocarSom(3);
            }
            break;
        case 'A':
            if (posicao > 0)
            {
                posicao--;
                mostrarGabarito = 0;
                tocarSom(1);
            }
            else
            {
                tocarSom(3);
            }
            break;
        case 'R':
            mostrarGabarito = !mostrarGabarito;
            tocarSom(mostrarGabarito ? 2 : 1);
            break;
        case 'S':
            tocarSom(1);
            break;
        default:
            tocarSom(3);
            break;
        }
    } while (comando != 'S');

    free(indices);
    limparTela();
}

void alterarPergunta(Banco *banco)
{
    if (banco->quantidade == 0)
    {
        tocarSom(3);
        printf("\n%s[!] Nenhuma pergunta cadastrada para alterar.%s\n", COR_AMARELO, COR_RESET);
        return;
    }

    int id;
    int leitura = lerInteiro("\nDigite o número (ID) da pergunta para alterar: ", &id);
    if (leitura == -1)
    {
        return;
    }

    int posicao = -1;
    for (int i = 0; i < banco->quantidade && leitura == 1; i++)
    {
        if (banco->dados[i].id == id)
        {
            posicao = i;
        }
    }
    if (posicao < 0)
    {
        tocarSom(3);
        printf("%s[!] ID inválido!%s\n", COR_VERMELHO, COR_RESET);
        return;
    }

    printf("\nPergunta atual:\n");
    imprimirPergunta(&banco->dados[posicao], 1);
    printf("\n--- NOVOS DADOS ---\n");

    // Preenche uma cópia: se a entrada fechar no meio, o original não é estragado
    Perguntas nova = banco->dados[posicao];
    if (!preencherPergunta(&nova))
    {
        return;
    }
    banco->dados[posicao] = nova;

    tocarSom(2);
    printf("%s[OK] Pergunta #%d alterada com sucesso!%s\n", COR_VERDE, id, COR_RESET);
}

void excluirPergunta(Banco *banco)
{
    if (banco->quantidade == 0)
    {
        tocarSom(3);
        printf("\n%s[!] Nenhuma pergunta cadastrada para excluir.%s\n", COR_AMARELO, COR_RESET);
        return;
    }

    int id;
    int leitura = lerInteiro("\nDigite o número (ID) da pergunta para excluir: ", &id);
    if (leitura == -1)
    {
        return;
    }

    int posicao = -1;
    for (int i = 0; i < banco->quantidade && leitura == 1; i++)
    {
        if (banco->dados[i].id == id)
        {
            posicao = i;
        }
    }
    if (posicao < 0)
    {
        tocarSom(3);
        printf("%s[!] ID inválido!%s\n", COR_VERMELHO, COR_RESET);
        return;
    }

    printf("\nPergunta a excluir:\n");
    imprimirPergunta(&banco->dados[posicao], 0);
    char resposta[8];
    if (!lerLinha("\nConfirma a exclusão? (S/N): ", resposta, sizeof(resposta)) ||
        toupper((unsigned char)resposta[0]) != 'S')
    {
        printf("Exclusão cancelada.\n");
        return;
    }

    // Puxa as perguntas seguintes uma posição para trás
    for (int i = posicao; i < banco->quantidade - 1; i++)
    {
        banco->dados[i] = banco->dados[i + 1];
    }
    banco->quantidade--;

    // Se sobrou pouca coisa, diminui o vetor pela metade
    if (banco->quantidade > 0 && banco->quantidade <= banco->capacidade / 4 &&
        banco->capacidade > CAPACIDADE_INICIAL)
    {
        int novaCapacidade = banco->capacidade / 2;
        Perguntas *temp = realloc(banco->dados, novaCapacidade * sizeof(Perguntas));
        if (temp != NULL)
        {
            banco->dados = temp;
            banco->capacidade = novaCapacidade;
        }
    }

    tocarSom(2);
    printf("%s[OK] Pergunta #%d excluída com sucesso!%s\n", COR_VERDE, id, COR_RESET);
}

void exportarPerguntas(const Banco *banco)
{
    char nome[TAM_CAMINHO];
    if (!lerLinha("\nNome do arquivo .csv (Enter = perguntas_exportadas.csv): ", nome, sizeof(nome)))
    {
        return;
    }
    if (nome[0] == '\0')
    {
        snprintf(nome, sizeof(nome), "%s", "perguntas_exportadas.csv");
    }

    if (exportarCSV(banco, nome) != 0)
    {
        tocarSom(3);
        printf("%s[!] Não foi possível criar '%s'.%s\n", COR_VERMELHO, nome, COR_RESET);
        return;
    }
    tocarSom(2);
    printf("%s[OK] %d pergunta(s) exportada(s) para '%s'.%s\n", COR_VERDE, banco->quantidade, nome, COR_RESET);
}
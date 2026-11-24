/**
 * @file sistema_cardGame.c
 * @author Guilherme
 * @brief Sistema de cartas do jogo: cria cartas a partir do banco de perguntas,
 *        sorteia por dificuldade e confere a resposta do jogador.
 * @version 0.3
 * @date 2026-11-23
 *
 * @copyright Copyright (c) 2026
 *
 */
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "jogo_demilhao.h"
#include "sistema_cardGame.h"

/**
 * @brief Sorteia um inteiro no intervalo fechado [min, max].
 *
 * @note Chame srand(time(NULL)) uma vez no main antes de usar esta funcao,
 *       senao a sequencia sorteada sera sempre a mesma.
 *
 * @param min Menor valor possivel (inclusive).
 * @param max Maior valor possivel (inclusive). Deve ser >= min.
 * @return Inteiro sorteado entre min e max.
 */
int sortear(int min, int max)
{
    return rand() % (max - min + 1) + min;
}

/**
 * @brief Converte as perguntas do banco em cartas do jogo.
 *
 * Para cada pergunta copia nivel, enunciado, alternativas e resposta correta,
 * e define o multiplicador (acerto) e o redutor (erro) conforme a dificuldade:
 * - MUITO_FACIL: MULTIPLICADOR_X2 / REDUTOR_X2
 * - FACIL: MULTIPLICADOR_X3 / REDUTOR_X3
 * - MEDIO: MULTIPLICADOR_X4 / REDUTOR_X4
 * - DIFICIL: MULTIPLICADOR_X5 / REDUTOR_X5
 * - MUITO_DIFICIL: MULTIPLICADOR_X6 / REDUTOR_X6
 *
 * Niveis fora de 1 a 5 recebem multiplicador 1 e redutor 1.0 (neutros).
 *
 * @param[out] cards   Vetor de cartas ja alocado com pelo menos @p tamanho posicoes.
 * @param[in]  tamanho Quantidade de perguntas a converter.
 * @param[in]  banco   Banco com as perguntas de origem.
 *
 * @warning Se @p cards ou @p banco forem NULL, a funcao apenas escreve uma
 *          mensagem em stderr e retorna sem fazer nada.
 */
void inicializarCards(CardGame *cards, int tamanho, Banco *banco)
{
    if (cards == NULL || banco == NULL)
    {
        fprintf(stderr, "Ponteiro nulo passado para inicializarCards\n");
        return;
    }

    for (int i = 0; i < tamanho; i++)
    {
        cards[i].nivel = banco->dados[i].nivel;
        cards[i].dificuldade = (enum Dificuldade)banco->dados[i].nivel;

        switch (cards[i].dificuldade)
        {
        case MUITO_FACIL:
            cards[i].numMultiplicador = MULTIPLICADOR_X2;
            cards[i].numRedutor = REDUTOR_X2;
            break;
        case FACIL:
            cards[i].numMultiplicador = MULTIPLICADOR_X3;
            cards[i].numRedutor = REDUTOR_X3;
            break;
        case MEDIO:
            cards[i].numMultiplicador = MULTIPLICADOR_X4;
            cards[i].numRedutor = REDUTOR_X4;
            break;
        case DIFICIL:
            cards[i].numMultiplicador = MULTIPLICADOR_X5;
            cards[i].numRedutor = REDUTOR_X5;
            break;
        case MUITO_DIFICIL:
            cards[i].numMultiplicador = MULTIPLICADOR_X6;
            cards[i].numRedutor = REDUTOR_X6;
            break;
        default:
            /* nivel invalido no banco: valores neutros */
            cards[i].numMultiplicador = 1;
            cards[i].numRedutor = 1.0;
            break;
        }

        strcpy(cards[i].enunciado, banco->dados[i].enunciado);
        for (int j = 0; j < 4; j++)
        {
            strcpy(cards[i].alternativa[j], banco->dados[i].alternativa[j]);
        }
        cards[i].correta = banco->dados[i].correta;
    }
}

/**
 * @brief Filtra as cartas de uma determinada dificuldade.
 *
 * Copia para @p vetorResultado todas as cartas de @p cards cuja dificuldade
 * seja igual a @p dificuldade, mantendo a ordem original.
 *
 * @param[in]  cards          Vetor com todas as cartas.
 * @param[in]  tamanho        Quantidade de cartas em @p cards.
 * @param[in]  dificuldade    Dificuldade procurada.
 * @param[out] vetorResultado Vetor que recebe as cartas encontradas. Deve ter
 *                            espaco para, no maximo, @p tamanho cartas.
 * @return Quantidade de cartas copiadas para @p vetorResultado (0 se nenhuma).
 */
int ProcurarDificuldadeCards(CardGame *cards, int tamanho, enum Dificuldade dificuldade, CardGame *vetorResultado)
{
    int tamanhoResultado = 0;
    for (int i = 0; i < tamanho; i++)
    {
        if (cards[i].dificuldade == dificuldade)
        {
            vetorResultado[tamanhoResultado] = cards[i];
            tamanhoResultado++;
        }
    }
    return tamanhoResultado;
}

/**
 * @brief Escolhe uma carta aleatoria dentro de um vetor de cartas.
 *
 * @param[in]  vetorResultado Vetor de cartas candidatas.
 * @param[out] CardEscolhido  Recebe uma copia da carta sorteada.
 * @param[in]  tamanhoResultado Quantidade de cartas em @p vetorResultado.
 *
 * @note Nao faz nada se algum ponteiro for NULL ou se
 *       @p tamanhoResultado for menor ou igual a zero.
 * @see sortear
 */
void EscolherCard(CardGame *vetorResultado, CardGame *CardEscolhido, int tamanhoResultado)
{
    if (vetorResultado != NULL && CardEscolhido != NULL && tamanhoResultado > 0)
    {
        int indiceSorteado = sortear(0, tamanhoResultado - 1);
        *CardEscolhido = vetorResultado[indiceSorteado];
    }
}

/**
 * @brief Informa na tela se a resposta do jogador esta correta.
 *
 * A comparacao ignora maiusculas e minusculas ('a' e 'A' valem o mesmo).
 *
 * @param[in] resposta       Letra escolhida pelo jogador.
 * @param[in] CardEscolhido  Carta que foi sorteada.
 */
void ConfereResposta(char resposta, CardGame *CardEscolhido)
{
    if (CardEscolhido != NULL)
    {
        if (toupper((unsigned char)resposta) == toupper((unsigned char)CardEscolhido->correta))
        {
            printf("Resposta correta!\n");
        }
        else
        {
            printf("Resposta incorreta! A resposta correta era: %c\n", CardEscolhido->correta);
        }
    }
}

/**
 * @brief Mostra a consequencia da resposta: multiplicador no acerto, redutor no erro.
 *
 * @param[in] resposta       Letra escolhida pelo jogador.
 * @param[in] CardEscolhido  Carta que foi sorteada.
 *
 * @note Esta funcao apenas exibe a mensagem; nao altera pontuacao nem estado.
 */
void ConsequenciaEscolha(char resposta, CardGame *CardEscolhido)
{
    if (CardEscolhido != NULL)
    {
        if (toupper((unsigned char)resposta) == toupper((unsigned char)CardEscolhido->correta))
        {
            printf("Voce ganhou um multiplicador de %d!\n", (int)CardEscolhido->numMultiplicador);
        }
        else
        {
            printf("Voce perdeu! Seu multiplicador foi reduzido para %.1f!\n", (double)CardEscolhido->numRedutor);
        }
    }
}

/**
 * @brief Libera a memoria de um vetor de cartas alocado dinamicamente.
 *
 * @param[in] cards Vetor alocado com malloc/calloc. Se for NULL, nada acontece.
 *
 * @warning Depois da chamada o ponteiro fica invalido; nao use @p cards novamente.
 */
void liberarCards(CardGame *cards)
{
    if (cards != NULL)
    {
        free(cards);
    }
}

/**
 * @brief Rodada de teste no terminal.
 *
 * Fluxo: monta as cartas a partir do banco, pede a dificuldade (1 a 5),
 * sorteia uma carta desse nivel, mostra o enunciado e as alternativas,
 * le a resposta e exibe o resultado e a consequencia.
 *
 * @param[in] banco Banco com as perguntas carregadas.
 *
 * @note Confira se o campo @c quantidade existe na struct Banco; se o nome
 *       for outro, ajuste a linha que define @c tamanho.
 * @see inicializarCards, ProcurarDificuldadeCards, EscolherCard,
 *      ConfereResposta, ConsequenciaEscolha
 */
void Teste(Banco *banco)
{
    if (banco == NULL)
    {
        fprintf(stderr, "Banco nulo passado para Teste\n");
        return;
    }

    int dificuldadeEscolhida;
    char alternativa;

    /* ATENCAO: confira o nome do campo de quantidade na struct Banco */
    int tamanho = banco->quantidade;
    if (tamanho <= 0)
    {
        printf("Banco de perguntas vazio.\n");
        return;
    }

    CardGame *carta = malloc(tamanho * sizeof(CardGame));
    if (carta == NULL)
    {
        fprintf(stderr, "Falha ao alocar memoria\n");
        return;
    }

    inicializarCards(carta, tamanho, banco);

    printf("Escolha a dificuldade (1 a 5): ");
    if (scanf("%d", &dificuldadeEscolhida) != 1 || dificuldadeEscolhida < MUITO_FACIL || dificuldadeEscolhida > MUITO_DIFICIL)
    {
        printf("Dificuldade invalida.\n");
        liberarCards(carta);
        return;
    }

    /* aloca com o tamanho total: sempre comporta os resultados */
    CardGame *vetorResultado = malloc(tamanho * sizeof(CardGame));
    if (vetorResultado == NULL)
    {
        fprintf(stderr, "Falha ao alocar memoria\n");
        liberarCards(carta);
        return;
    }

    int tamanhoResultado = ProcurarDificuldadeCards(carta, tamanho, (enum Dificuldade)dificuldadeEscolhida, vetorResultado);
    if (tamanhoResultado == 0)
    {
        printf("Nenhuma pergunta encontrada para essa dificuldade.\n");
        liberarCards(vetorResultado);
        liberarCards(carta);
        return;
    }

    CardGame cartaEscolhida;
    EscolherCard(vetorResultado, &cartaEscolhida, tamanhoResultado);

    printf("Pergunta: %s\n", cartaEscolhida.enunciado);
    for (int i = 0; i < 4; i++)
    {
        printf("%s\n", cartaEscolhida.alternativa[i]);
    }

    printf("Resposta: ");
    scanf(" %c", &alternativa); /* espaco antes do %c ignora o Enter pendente */

    ConfereResposta(alternativa, &cartaEscolhida);
    ConsequenciaEscolha(alternativa, &cartaEscolhida);

    liberarCards(vetorResultado);
    liberarCards(carta);
}
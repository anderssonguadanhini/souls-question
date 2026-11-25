/**
 * @file interface.c
 * @brief Interface gráfica (raylib): arena com herói, inimigos e itens, e o
 *        sistema de cartas de perguntas que altera o poder do herói.
 *
 * O jogador clica para mover o herói, coletar itens e enfrentar inimigos.
 * Vencer um combate (poder do herói >= poder do inimigo) abre a escolha de
 * cartas; acertar multiplica o poder e errar reduz.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "raylib.h"
#include <ctype.h>
#include "interface.h"
#include "jogo_demilhao.h"
#include "sistema_cardGame.h"

#define MAX_INIMIGOS 16           ///< Capacidade do vetor de inimigos da arena.
#define MAX_ITENS 12              ///< Capacidade do vetor de itens da arena.
#define MAX_POPUPS 12             ///< Quantos textos flutuantes podem aparecer ao mesmo tempo.
#define PODER_MAX_INIMIGO 9999999 ///< Maior poder de um inimigo; o herói pode passar disso (ver clampIntMax).
#define PODER_INICIAL_HEROI 5     ///< Poder com que o herói começa toda arena.

/** @brief Tipos de item que podem ser coletados na arena. */
typedef enum
{
    TIPO_ITEM_BAU = 0,  ///< Baú: SOMA um bônus ao poder.
    TIPO_ITEM_ARMA_MULT ///< Espada: MULTIPLICA o poder.
} TipoItem;

/** @brief Estado do herói controlado pelo jogador (posição, poder, animações e sprites). */
typedef struct
{
    Vector2 pos;               ///< Posição atual (base/pés do personagem).
    Vector2 destino;           ///< Para onde o herói está andando.
    int movendo;               ///< 1 enquanto anda até o destino.
    int direcaoX;              ///< 1 = direita, -1 = esquerda.
    float velocidade;          ///< Velocidade de caminhada em pixels por segundo.
    int inimigoAlvoIdx;        ///< Índice do inimigo que vai atacar (-1 = nenhum).
    int itemAlvoIdx;           ///< Índice do item que vai pegar (-1 = nenhum).
    float timerAtaque;         ///< Tempo restante da animação de golpe.
    float timerRecuo;          ///< Tempo restante da animação de dano (perdeu o combate).
    float timerLevelUp;        ///< Tempo restante do efeito visual de evolução.
    int poder;                 ///< Poder atual (comparado com o do inimigo no combate).
    int nivelArma;             ///< Quantidade de itens coletados.
    int estagioEvolucao;       ///< 0: hero, 1: hero_espada1, 2: hero_espada2, 3: hero_final.
    int derrotouSif;           ///< 1 após derrotar o Sif (libera a forma final).
    int vivo;                  ///< 1 enquanto o herói pode agir.
    Texture2D texHeroi;        ///< Sprite do estágio 0.
    int temTexHeroi;           ///< 1 se texHeroi foi carregado.
    Texture2D texHeroiEspada1; ///< Sprite do estágio 1.
    int temTexHeroiEspada1;    ///< 1 se texHeroiEspada1 foi carregado.
    Texture2D texHeroiEspada2; ///< Sprite do estágio 2.
    int temTexHeroiEspada2;    ///< 1 se texHeroiEspada2 foi carregado.
    Texture2D texHeroiFinal;   ///< Sprite do estágio 3 (Forma Suprema).
    int temTexHeroiFinal;      ///< 1 se texHeroiFinal foi carregado.
} Heroi;

/** @brief Um inimigo da arena (Hollows e chefes). */
typedef struct
{
    int id;               ///< Identificador (também desincroniza as animações).
    char nome[48];        ///< Nome mostrado na tela.
    Vector2 pos;          ///< Posição (base/pés).
    int poder;            ///< Poder necessário para vencê-lo.
    int nivelDificuldade; ///< Dificuldade de 1 a 5 (usada no visual).
    int derrotado;        ///< 1 depois de vencido (some da arena).
    int ehChefe;          ///< 1 para chefes (Sif e Gwyn): maiores e com hitbox maior.
    Texture2D texInimigo; ///< Sprite do inimigo.
    int temTexInimigo;    ///< 1 se texInimigo foi carregado.
} Inimigo;

/** @brief Um item coletável (baú ou espada). */
typedef struct
{
    int id;            ///< Identificador do item.
    char nome[48];     ///< Nome mostrado na tela.
    TipoItem tipo;     ///< Baú (soma) ou espada (multiplica).
    Vector2 pos;       ///< Posição na arena.
    int bonusPoder;    ///< Quanto o baú soma ao poder (ex.: +4, +10).
    int multiplicador; ///< Quanto a espada multiplica o poder (ex.: x4, x100).
    int coletado;      ///< 1 depois de coletado (some da arena).
    Texture2D texItem; ///< Sprite do item (opcional).
    int temTexItem;    ///< 1 se texItem foi carregado.
} Item;

/** @brief Imagem de fundo da arena. */
typedef struct
{
    Texture2D texCenario; ///< Textura do cenário.
    int temCenario;       ///< 1 se texCenario foi carregado.
} Cenario;

/** @brief Tudo o que existe em uma partida gráfica: herói, inimigos, itens e cenário. */
typedef struct
{
    int limiarEspada1;              ///< Poder que libera o estágio visual 1.
    int limiarEspada2;              ///< Poder que libera o estágio visual 2.
    int limiarFormaFinal;           ///< Poder que libera a Forma Suprema.
    Cenario cenario;                ///< Fundo da arena.
    Heroi heroi;                    ///< O jogador.
    Inimigo inimigos[MAX_INIMIGOS]; ///< Inimigos da arena.
    int qtdInimigos;                ///< Quantos inimigos estão em uso.
    Item itens[MAX_ITENS];          ///< Itens da arena.
    int qtdItens;                   ///< Quantos itens estão em uso.
    int vitoriaArena;               ///< 1 quando todos os inimigos foram derrotados.
} Arena;

/** @brief Deformações aplicadas ao sprite para simular respiração e passos. */
typedef struct
{
    float escalaX;      ///< Escala horizontal do sprite.
    float escalaY;      ///< Escala vertical do sprite.
    float passoY;       ///< Deslocamento vertical (pulinho do passo).
    float inclinacao;   ///< Rotação em graus.
    float sombraEscala; ///< Escala da sombra no chão.
} MovimentoCorpo;

/** @brief Texto que sobe e some na tela (ex.: "+10 PODER!"). */
typedef struct
{
    int ativo;           ///< 1 enquanto está sendo mostrado.
    char texto[48];      ///< Mensagem.
    Vector2 pos;         ///< Posição atual (sobe com o tempo).
    Color cor;           ///< Cor do texto.
    float tempoRestante; ///< Segundos até sumir.
} TextoFlutuante;

/** @brief Textos flutuantes ativos (static: visível só neste arquivo). */
static TextoFlutuante popups[MAX_POPUPS] = {0};

/**
 * @brief Limita um valor ao intervalo [minimo, maximo].
 *
 * Recebe long long para que as contas de poder (multiplicações) não
 * estourem o int antes de serem limitadas.
 *
 * @param valor  Valor a limitar.
 * @param minimo Menor valor permitido.
 * @param maximo Maior valor permitido.
 * @return O valor dentro do intervalo, já convertido para int.
 */
static int clampIntMax(long long valor, int minimo, int maximo)
{
    if (valor < minimo)
        return minimo;
    if (valor > maximo)
        return maximo;
    return (int)valor;
}

/**
 * @brief Sorteia um inteiro em [minimo, maximo], mesmo para faixas grandes.
 *
 * O rand() do Windows tem só 15 bits (até 32767), então dois sorteios são
 * combinados para cobrir faixas de até ~10 milhões.
 *
 * @param minimo Menor valor possível.
 * @param maximo Maior valor possível.
 * @return Valor sorteado (ou @p minimo se maximo <= minimo).
 */
static int sortearPoderAteMaximo(int minimo, int maximo)
{
    if (maximo <= minimo)
        return minimo;
    // rand() pode ter só 15 bits (Windows); junta dois sorteios para cobrir faixas até ~10 milhões
    unsigned int r1 = (unsigned int)(rand() & 0xFFFF);
    unsigned int r2 = (unsigned int)(rand() & 0xFFFF);
    unsigned int combinado = (r1 << 16) | r2;
    unsigned int intervalo = (unsigned int)(maximo - minimo + 1);
    return minimo + (int)(combinado % intervalo);
}

/**
 * @brief Sorteia uma posição na tela longe das posições já ocupadas.
 *
 * Tenta até 80 posições; se nenhuma ficar longe o bastante, fica com a
 * última sorteada (melhor sobrepor do que travar). A posição escolhida é
 * acrescentada a @p ocupados (até 32 posições).
 *
 * @param ocupados    Vetor de posições já usadas (capacidade 32).
 * @param qtdOcupados Quantidade de posições em @p ocupados (é atualizada).
 * @param largTela    Largura da tela em pixels.
 * @param altTela     Altura da tela em pixels.
 * @param distMinima  Distância mínima até as posições ocupadas.
 * @return A posição sorteada.
 */
static Vector2 sortearPosicaoLivre(Vector2 ocupados[], int *qtdOcupados, float largTela, float altTela, float distMinima)
{
    Vector2 candidata = {largTela * 0.5f, altTela * 0.55f};

    // Tenta até 80 posições; se nenhuma ficar longe o bastante dos ocupados,
    // fica com a última sorteada (melhor sobrepor do que travar).
    for (int tentativa = 0; tentativa < 80; tentativa++)
    {
        float rx = (float)GetRandomValue(16, 84) / 100.0f;
        float ry = (float)GetRandomValue(34, 80) / 100.0f;
        candidata = (Vector2){largTela * rx, altTela * ry};

        int valida = 1;
        for (int i = 0; i < *qtdOcupados; i++)
        {
            float dx = candidata.x - ocupados[i].x;
            float dy = candidata.y - ocupados[i].y;
            if (sqrtf(dx * dx + dy * dy) < distMinima)
            {
                valida = 0;
                break;
            }
        }

        if (valida)
            break;
    }

    // limite do vetor "ocupados" declarado com 32 posições
    if (*qtdOcupados < 32)
    {
        ocupados[*qtdOcupados] = candidata;
        (*qtdOcupados)++;
    }
    return candidata;
}

/**
 * @brief Mostra um texto flutuante que sobe e desaparece.
 *
 * Usa a primeira vaga livre de ::popups; se todas estiverem em uso, o
 * aviso é descartado.
 *
 * @param texto Mensagem (até 47 caracteres).
 * @param pos   Posição inicial na tela.
 * @param cor   Cor do texto.
 */
static void adicionarPopup(const char *texto, Vector2 pos, Color cor)
{
    for (int i = 0; i < MAX_POPUPS; i++)
    {
        if (!popups[i].ativo)
        {
            popups[i].ativo = 1;
            snprintf(popups[i].texto, sizeof(popups[i].texto), "%s", texto);
            popups[i].pos = pos;
            popups[i].cor = cor;
            popups[i].tempoRestante = 1.5f;
            return;
        }
    }
}

/**
 * @brief Move, esmaece e desenha os textos flutuantes ativos.
 * @param dt Tempo do quadro em segundos.
 */
static void atualizarEDesenharPopups(float dt)
{
    for (int i = 0; i < MAX_POPUPS; i++)
    {
        if (!popups[i].ativo)
            continue;

        popups[i].tempoRestante -= dt;
        popups[i].pos.y -= 38.0f * dt;

        if (popups[i].tempoRestante <= 0.0f)
        {
            popups[i].ativo = 0;
            continue;
        }

        unsigned char alpha = (popups[i].tempoRestante < 0.45f)
                                  ? (unsigned char)((popups[i].tempoRestante / 0.45f) * 255.0f)
                                  : 255;

        Color c = popups[i].cor;
        c.a = alpha;
        Color sombra = {0, 0, 0, alpha};

        int larg = MeasureText(popups[i].texto, 26);
        int x = (int)(popups[i].pos.x - larg / 2.0f);
        int y = (int)popups[i].pos.y;

        DrawText(popups[i].texto, x + 2, y + 2, 26, sombra);
        DrawText(popups[i].texto, x, y, 26, c);
    }
}

/**
 * @brief Carrega uma imagem como textura, deixando o fundo transparente.
 *
 * Se o canto superior esquerdo for branco, remove o fundo branco; senão,
 * remove o fundo preto puro.
 *
 * @param caminho   Caminho da imagem.
 * @param carregado Recebe 1 se a textura foi criada, 0 se não.
 * @return A textura (id 0 se o arquivo não existe ou falhou).
 */
static Texture2D carregarSpriteRecortado(const char *caminho, int *carregado)
{
    *carregado = 0;
    Texture2D tex = {0};
    if (!FileExists(caminho))
        return tex;

    Image img = LoadImage(caminho);
    if (img.data == NULL)
        return tex;

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *pixels = LoadImageColors(img);
    int total = img.width * img.height;

    int fundoBranco = (total > 0 && pixels[0].a > 200 && pixels[0].r > 240 && pixels[0].g > 240 && pixels[0].b > 240);

    for (int i = 0; i < total; i++)
    {
        if (fundoBranco)
        {
            if (pixels[i].r > 240 && pixels[i].g > 240 && pixels[i].b > 240)
            {
                pixels[i].a = 0;
            }
        }
        else
        {
            if (pixels[i].r < 14 && pixels[i].g < 14 && pixels[i].b < 14)
            {
                pixels[i].a = 0;
            }
        }
    }

    // UnloadImage libera só os dados originais; "pixels" é uma cópia separada
    // (já com o fundo transparente) e passa a ser a imagem usada na textura.
    UnloadImage(img);
    img.data = pixels;
    img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    img.mipmaps = 1;

    tex = LoadTextureFromImage(img);
    UnloadImageColors(pixels);

    if (tex.id > 0)
    {
        *carregado = 1;
    }
    return tex;
}

/**
 * @brief Calcula a "respiração" (parado) ou os passos (andando) de um personagem.
 * @param ehHeroi   1 para o herói (ritmo diferente dos inimigos).
 * @param movendo   1 se está andando.
 * @param direcaoX  1 = direita, -1 = esquerda (define a inclinação).
 * @param faseTempo Atraso para os personagens não se moverem em sincronia.
 * @return As deformações a aplicar ao sprite.
 */
static MovimentoCorpo calcularMovimentoPersonagem(int ehHeroi, int movendo, int direcaoX, float faseTempo)
{
    MovimentoCorpo mov = {1.0f, 1.0f, 0.0f, 0.0f, 1.0f};
    float t = (float)GetTime() + faseTempo;

    if (movendo)
    {
        float cicloPasso = fabsf(sinf(t * 14.0f));
        float balanco = sinf(t * 14.0f);

        mov.passoY = -cicloPasso * 8.0f;
        mov.escalaY = 1.0f + (cicloPasso * 0.035f);
        mov.escalaX = 1.0f - (cicloPasso * 0.025f);
        mov.inclinacao = (direcaoX * 4.5f) + (balanco * 2.2f);
        mov.sombraEscala = 1.0f - (cicloPasso * 0.14f);
    }
    else
    {
        float resp = sinf(t * (ehHeroi ? 3.2f : 2.6f));
        mov.passoY = 0.0f;
        mov.escalaY = 1.0f + (resp * 0.024f);
        mov.escalaX = 1.0f - (resp * 0.012f);
        mov.inclinacao = ehHeroi ? (sinf(t * 1.6f) * 1.2f) : (sinf(t * 2.1f) * 1.5f);
        mov.sombraEscala = 1.0f;
    }

    return mov;
}

/**
 * @brief Atualiza o estágio visual do herói (0 -> 1 -> 2 -> 3).
 *
 * A Forma Suprema (3) é ativada ao derrotar o Sif ou ao atingir o poder
 * necessário antes de enfrentar o Gwyn. O estágio só sobe, nunca volta.
 *
 * @param arena         Arena com o herói e os limiares de evolução.
 * @param forcarEspada2 1 para garantir pelo menos o estágio 2 (pegou a espada).
 */
static void atualizarEvolucaoHeroi(Arena *arena, int forcarEspada2)
{
    Heroi *heroi = &arena->heroi;
    int novoEstagio = heroi->estagioEvolucao;

    if (heroi->derrotouSif || heroi->poder >= arena->limiarFormaFinal)
    {
        novoEstagio = 3; // hero_final.png (Forma Suprema)
    }
    else if (forcarEspada2 || heroi->nivelArma >= 2 || heroi->poder >= arena->limiarEspada2)
    {
        if (novoEstagio < 2)
            novoEstagio = 2; // hero_espada2.png
    }
    else if (heroi->nivelArma >= 1 || heroi->poder >= arena->limiarEspada1)
    {
        if (novoEstagio < 1)
            novoEstagio = 1; // hero_espada1.png
    }

    // O estágio só sobe, nunca volta (mesmo que o poder caia por erro numa carta)
    if (novoEstagio > heroi->estagioEvolucao)
    {
        heroi->estagioEvolucao = novoEstagio;
        heroi->timerLevelUp = 1.05f;

        if (novoEstagio == 3)
        {
            adicionarPopup("FORMA SUPREMA! ARTORIAS & SIF",
                           (Vector2){heroi->pos.x, heroi->pos.y - 205.0f},
                           (Color){255, 215, 70, 255});
        }
        else if (novoEstagio == 2)
        {
            adicionarPopup("EVOLUCAO II! ESPADA DO ABISMO",
                           (Vector2){heroi->pos.x, heroi->pos.y - 190.0f},
                           (Color){130, 220, 255, 255});
        }
        else if (novoEstagio == 1)
        {
            adicionarPopup("EVOLUCAO I! ESPADA EQUIPADA",
                           (Vector2){heroi->pos.x, heroi->pos.y - 185.0f},
                           (Color){115, 235, 150, 255});
        }
    }
}

/**
 * @brief Aplica o efeito de um item coletado e evolui o herói.
 *
 * Baú soma @c bonusPoder ao poder; espada multiplica o poder por
 * @c multiplicador.
 *
 * @param arena   Arena com o herói e os itens.
 * @param idxItem Índice do item em @c arena->itens.
 */
static void coletarItem(Arena *arena, int idxItem)
{
    Item *it = &arena->itens[idxItem];
    if (it->coletado)
        return;

    it->coletado = 1;
    arena->heroi.timerLevelUp = 0.85f;
    arena->heroi.nivelArma++; // conta itens coletados; também define o estágio visual

    if (it->tipo == TIPO_ITEM_ARMA_MULT)
    {
        // espada: MULTIPLICA o poder (baús, abaixo, apenas somam)
        long long novoPoder = (long long)arena->heroi.poder * (long long)it->multiplicador;
        arena->heroi.poder = clampIntMax(novoPoder, 1, PODER_MAX_INIMIGO * 2);
        adicionarPopup(TextFormat("PODER x%d!", it->multiplicador),
                       (Vector2){arena->heroi.pos.x, arena->heroi.pos.y - 170.0f},
                       (Color){255, 220, 70, 255});
        atualizarEvolucaoHeroi(arena, 1);
    }
    else
    {
        long long novoPoder = (long long)arena->heroi.poder + (long long)it->bonusPoder;
        arena->heroi.poder = clampIntMax(novoPoder, 1, PODER_MAX_INIMIGO * 2);
        adicionarPopup(TextFormat("+%d PODER!", it->bonusPoder),
                       (Vector2){arena->heroi.pos.x, arena->heroi.pos.y - 170.0f},
                       (Color){100, 235, 130, 255});
        atualizarEvolucaoHeroi(arena, 0);
    }
}

// ============================================================================
// SISTEMA DE CARTAS
// Apos vencer um combate o jogador escolhe 1 de 5 cartas (uma por dificuldade),
// responde a pergunta sorteada e recebe o multiplicador (acerto) ou o redutor
// (erro) sobre o poder. Reaproveita o sistema_cardGame.c:
// inicializarCards, ProcurarDificuldadeCards e EscolherCard.
// ============================================================================
#define CARTAS_QTD 5    ///< Uma carta por nível de dificuldade.
#define CARTA_TEX_L 360 ///< Largura (px) da textura de uma carta.
#define CARTA_TEX_A 504 ///< Altura (px) da textura de uma carta.

/** @brief Fases da tela de cartas, na ordem em que acontecem. */
typedef enum
{
    CARTAS_FASE_INATIVA = 0, ///< Nenhuma carta na tela (o jogador anda pela arena).
    CARTAS_FASE_ESPERA,      ///< Pequena pausa para ver o "+poder" do combate.
    CARTAS_FASE_ESCOLHA,     ///< 5 cartas de costas na mesa.
    CARTAS_FASE_REVELANDO,   ///< Carta escolhida vai ao centro e desvira.
    CARTAS_FASE_PERGUNTA,    ///< Enunciado + alternativas A-D.
    CARTAS_FASE_RESULTADO    ///< Acerto/erro e consequência no poder.
} FaseCartas;

/** @brief Estado da tela de cartas durante uma escolha/pergunta. */
typedef struct
{
    FaseCartas fase;            ///< Fase atual.
    float tempoFase;            ///< Segundos desde que a fase começou.
    int escolhida;              ///< Índice (0-4) da carta escolhida, ou -1.
    int hover;                  ///< Carta ou botão sob o mouse, ou -1.
    int disponivel[CARTAS_QTD]; ///< 1 se o nível tem pelo menos uma pergunta.
    float elevacao[CARTAS_QTD]; ///< Animação de "levantar" a carta no hover (0 a 1).
    CardGame carta;             ///< Pergunta sorteada.
    char letraEscolhida;        ///< Resposta do jogador ('A' a 'D').
    int acertou;                ///< 1 se acertou a pergunta.
    int poderAntes;             ///< Poder do herói antes da resposta.
    int poderDepois;            ///< Poder do herói depois da resposta.
} EstadoCartas;

/** @brief Paleta e efeitos visuais de uma carta (cada carta homenageia um chefe). */
typedef struct
{
    Color fundoTopo;      ///< Cor do topo do degradê de fundo.
    Color fundoBase;      ///< Cor da base do degradê de fundo.
    Color moldura;        ///< Cor da moldura externa.
    Color molduraInterna; ///< Cor da moldura interna.
    Color texto;          ///< Cor das letras.
    Color contorno;       ///< Cor do contorno das letras.
    Color brilho;         ///< Cor do brilho em volta das letras.
    float espacamento;    ///< Espaço entre letras.
    float ondulacao;      ///< Quanto as letras sobem e descem.
    int fonte;            ///< Tamanho da palavra principal.
    int pingos;           ///< 1 = letras "escorrendo" (podridão escarlate).
    int contornoDuplo;    ///< 1 = segundo contorno dourado.
} EstiloCarta;

/** @brief Posições calculadas do painel de pergunta (depende do tamanho da tela e do texto). */
typedef struct
{
    Rectangle painel;      ///< Retângulo do painel inteiro.
    Rectangle botoes[4];   ///< Botões das alternativas A-D.
    float yCabecalho;      ///< Y do cabeçalho.
    float yEnunciado;      ///< Y do enunciado.
    float alturaEnunciado; ///< Altura ocupada pelo enunciado já quebrado em linhas.
    float yRodape;         ///< Y do rodapé.
} LayoutPergunta;

/* Estado do sistema de cartas. São static (visíveis só neste arquivo) porque
 * as texturas precisam existir entre um quadro e outro do laço do raylib. */
static EstadoCartas estadoCartas;                ///< Estado da tela de cartas.
static CardGame *todasCartas = NULL;             ///< Todas as perguntas convertidas em cartas.
static CardGame *filtroCartas = NULL;            ///< Vetor de trabalho para filtrar por nível.
static int totalCartas = 0;                      ///< Tamanho de todasCartas.
static RenderTexture2D costasCartas[CARTAS_QTD]; ///< Costas desenhadas de cada carta.
static int temCostasCartas[CARTAS_QTD];          ///< 1 se a costa foi gerada.
static Texture2D artesCartas[CARTAS_QTD];        ///< Arte opcional (assets/cartas/<boss>.png).
static int temArteCarta[CARTAS_QTD];             ///< 1 se a arte foi carregada.
static Font fontesCartas[CARTAS_QTD];            ///< Fonte opcional (assets/fonts/<boss>.ttf).
static int temFonteCarta[CARTAS_QTD];            ///< 1 se a fonte foi carregada.

/** @brief Nível de dificuldade de cada uma das 5 cartas. */
static const enum Dificuldade ordemDificuldades[CARTAS_QTD] = {MUITO_FACIL, FACIL, MEDIO, DIFICIL, MUITO_DIFICIL};
/** @brief Texto da dificuldade escrito nas costas de cada carta. */
static const char *rotulosDificuldade[CARTAS_QTD] = {"MUITO FACIL", "FACIL", "MEDIO", "DIFICIL", "MUITO DIFICIL"};
/** @brief Chefe homenageado por cada carta. */
static const char *nomesBossCarta[CARTAS_QTD] = {"MARGIT", "RENNALA", "RADAHN", "RADAGON", "MALENIA"};
/** @brief Nome dos arquivos opcionais de arte/fonte de cada carta. */
static const char *arquivosBossCarta[CARTAS_QTD] = {"margit", "rennala", "radahn", "radagon", "malenia"};
/** @brief Numeral romano de cada carta. */
static const char *numeraisCarta[CARTAS_QTD] = {"I", "II", "III", "IV", "V"};

/** @brief Estilo visual de cada carta (mesma ordem de ordemDificuldades). */
static const EstiloCarta estilosCartas[CARTAS_QTD] = {
    // 0 - Margit: ferro velho e ouro da Ordem Dourada
    {{62, 60, 55, 255}, {20, 18, 15, 255}, {200, 164, 88, 255}, {126, 102, 54, 255}, {232, 220, 182, 255}, {18, 14, 8, 255}, {200, 164, 88, 255}, 3.0f, 0.0f, 40, 0, 0},
    // 1 - Rennala: noite azul e lua cheia
    {{22, 34, 84, 255}, {6, 8, 30, 255}, {172, 202, 255, 255}, {92, 122, 204, 255}, {228, 240, 255, 255}, {8, 14, 54, 255}, {150, 190, 255, 255}, 4.0f, 3.0f, 38, 0, 0},
    // 2 - Radahn: estrelas e brasa
    {{128, 40, 12, 255}, {30, 6, 6, 255}, {242, 152, 54, 255}, {172, 92, 32, 255}, {255, 204, 114, 255}, {52, 10, 0, 255}, {255, 120, 30, 255}, 3.0f, 0.0f, 42, 0, 0},
    // 3 - Radagon: dourado puro
    {{116, 88, 22, 255}, {32, 22, 6, 255}, {255, 222, 104, 255}, {192, 152, 52, 255}, {255, 240, 164, 255}, {72, 46, 0, 255}, {255, 222, 90, 255}, 3.0f, 0.0f, 40, 0, 1},
    // 4 - Malenia: podridao escarlate
    {{128, 14, 18, 255}, {30, 0, 4, 255}, {226, 62, 52, 255}, {142, 26, 26, 255}, {255, 154, 124, 255}, {42, 0, 0, 255}, {255, 60, 40, 255}, 2.0f, 2.0f, 38, 1, 0}};

// ----------------------------------------------------------------------------
// Utilidades
// ----------------------------------------------------------------------------
/**
 * @brief Limita um valor ao intervalo [0, 1].
 * @param v Valor.
 * @return @p v limitado a [0, 1].
 */
static float cartasClamp01(float v)
{
    if (v < 0.0f)
        return 0.0f;
    if (v > 1.0f)
        return 1.0f;
    return v;
}

/**
 * @brief Curva de animação "ease-out" (começa rápido, termina devagar).
 * @param t Progresso de 0 a 1.
 * @return Progresso suavizado de 0 a 1.
 */
static float cartasSuavizar(float t)
{
    t = cartasClamp01(t);
    return 1.0f - powf(1.0f - t, 3.0f);
}

/**
 * @brief Gera um número pseudoaleatório fixo para cada semente.
 *
 * Não usa rand(), para não alterar a sequência de sorteios do jogo e para
 * os desenhos das cartas saírem sempre iguais.
 *
 * @param semente Semente.
 * @return Valor entre 0 e 1.
 */
static float cartasRuido(int semente)
{
    unsigned int x = (unsigned int)semente * 747796405u + 2891336453u;
    x = ((x >> ((x >> 28u) + 4u)) ^ x) * 277803737u;
    x = (x >> 22u) ^ x;
    return (float)(x & 0xFFFFu) / 65535.0f;
}

/**
 * @brief Desenha um triângulo em qualquer ordem de vértices.
 *
 * O raylib descarta triângulos no sentido horário; esta função corrige a ordem.
 *
 * @param a   Vértice 1.
 * @param b   Vértice 2.
 * @param c   Vértice 3.
 * @param cor Cor de preenchimento.
 */
static void cartasTri(Vector2 a, Vector2 b, Vector2 c, Color cor)
{
    float cruz = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (cruz > 0.0f)
        DrawTriangle(a, c, b, cor);
    else
        DrawTriangle(a, b, c, cor);
}

/**
 * @brief Desenha um quadrilátero como dois triângulos.
 * @param a   Vértice 1.
 * @param b   Vértice 2.
 * @param c   Vértice 3.
 * @param d   Vértice 4.
 * @param cor Cor de preenchimento.
 */
static void cartasQuad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color cor)
{
    cartasTri(a, b, c, cor);
    cartasTri(a, c, d, cor);
}

/**
 * @brief Desenha um arco com espessura variável (chifres, anéis, cabelos...).
 * @param cx     Centro X.
 * @param cy     Centro Y.
 * @param raio   Raio do arco.
 * @param a0     Ângulo inicial em graus.
 * @param a1     Ângulo final em graus.
 * @param espIni Espessura no início.
 * @param espFim Espessura no fim.
 * @param cor    Cor do arco.
 */
static void cartasArcoAfilado(float cx, float cy, float raio, float a0, float a1, float espIni, float espFim, Color cor)
{
    const int passos = 32;
    Vector2 ant = {cx + cosf(a0 * DEG2RAD) * raio, cy + sinf(a0 * DEG2RAD) * raio};
    for (int i = 1; i <= passos; i++)
    {
        float t = (float)i / (float)passos;
        float a = (a0 + (a1 - a0) * t) * DEG2RAD;
        Vector2 p = {cx + cosf(a) * raio, cy + sinf(a) * raio};
        float esp = espIni + (espFim - espIni) * t;
        DrawLineEx(ant, p, esp, cor);
        DrawCircleV(p, esp * 0.5f, cor);
        ant = p;
    }
}

/**
 * @brief Espalha estrelas (pontos de brilho) na arte de uma carta.
 * @param semente Semente que define a posição das estrelas.
 * @param qtd     Quantidade de estrelas.
 * @param cor     Cor das estrelas.
 * @param yMax    Altura máxima onde podem aparecer.
 */
static void cartasEstrelas(int semente, int qtd, Color cor, float yMax)
{
    for (int i = 0; i < qtd; i++)
    {
        float x = 28.0f + cartasRuido(semente * 131 + i * 2) * (CARTA_TEX_L - 56.0f);
        float y = 28.0f + cartasRuido(semente * 131 + i * 2 + 1) * (yMax - 28.0f);
        float r = 0.8f + cartasRuido(semente * 17 + i) * 2.0f;
        DrawCircleV((Vector2){x, y}, r, Fade(cor, 0.35f + cartasRuido(semente * 7 + i) * 0.6f));
    }
}

/**
 * @brief Desenha (ou só mede) um texto quebrando as linhas por largura.
 * @param texto       Texto a desenhar.
 * @param x           X inicial (ou centro, se @p centralizar).
 * @param y           Y da primeira linha.
 * @param larguraMax  Largura máxima de cada linha.
 * @param fonte       Tamanho da fonte.
 * @param cor         Cor do texto.
 * @param centralizar 1 para centralizar cada linha em @p x.
 * @param desenhar    0 para apenas medir, sem desenhar.
 * @return Altura total ocupada, em pixels.
 */
static int cartasTextoQuebrado(const char *texto, float x, float y, float larguraMax, int fonte, Color cor, int centralizar, int desenhar)
{
    char linha[512] = {0};
    int altLinha = fonte + 8;
    float yAtual = y;
    const char *p = texto;

    while (*p)
    {
        char palavra[96];
        int n = 0;
        while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')
            p++;
        while (*p && *p != ' ' && *p != '\n' && *p != '\r' && *p != '\t' && n < 95)
            palavra[n++] = *p++;
        palavra[n] = '\0';
        if (n == 0)
            break;

        char teste[640]; /* linha (ate 511) + espaco + palavra (ate 95) */
        if (linha[0])
            snprintf(teste, sizeof(teste), "%s %s", linha, palavra);
        else
            snprintf(teste, sizeof(teste), "%s", palavra);

        size_t tamTeste = strlen(teste);
        if (linha[0] && ((float)MeasureText(teste, fonte) > larguraMax || tamTeste >= sizeof(linha)))
        {
            if (desenhar)
            {
                float lx = centralizar ? x - MeasureText(linha, fonte) / 2.0f : x;
                DrawText(linha, (int)lx, (int)yAtual, fonte, cor);
            }
            yAtual += altLinha;
            snprintf(linha, sizeof(linha), "%s", palavra);
        }
        else
        {
            if (tamTeste >= sizeof(linha))
                tamTeste = sizeof(linha) - 1;
            memcpy(linha, teste, tamTeste);
            linha[tamTeste] = '\0';
        }
    }

    if (linha[0])
    {
        if (desenhar)
        {
            float lx = centralizar ? x - MeasureText(linha, fonte) / 2.0f : x;
            DrawText(linha, (int)lx, (int)yAtual, fonte, cor);
        }
        yAtual += altLinha;
    }
    return (int)(yAtual - y);
}

/**
 * @brief Remove o prefixo "A) ", "B. " etc. de uma alternativa.
 *
 * A letra já é desenhada no botão, então o prefixo ficaria repetido.
 *
 * @param alt Texto da alternativa.
 * @return Ponteiro para o texto sem o prefixo (dentro de @p alt).
 */
static const char *cartasTextoAlternativa(const char *alt)
{
    if (alt[0] != '\0' && alt[1] != '\0' && strchr(").]:-", alt[1]) != NULL &&
        toupper((unsigned char)alt[0]) >= 'A' && toupper((unsigned char)alt[0]) <= 'D')
    {
        const char *p = alt + 2;
        while (*p == ' ')
            p++;
        return p;
    }
    return alt;
}

/**
 * @brief Fator que multiplica o poder quando o jogador acerta a carta.
 * @param carta Carta respondida.
 * @return O multiplicador da carta (1 se for inválido).
 */
static double cartasFatorAcerto(const CardGame *carta)
{
    double m = (double)carta->numMultiplicador;
    return (m > 0.0) ? m : 1.0;
}

/**
 * @brief Fator que multiplica o poder quando o jogador erra a carta.
 *
 * O redutor é aplicado como fator (ex.: 0.5 = metade); se vier maior que 1,
 * vira divisor (redutor 4 -> fator 0.25).
 *
 * @param carta Carta respondida.
 * @return Fator entre 0 e 1 (1 se o redutor for inválido).
 */
static double cartasFatorErro(const CardGame *carta)
{
    double r = (double)carta->numRedutor;
    if (r <= 0.0)
        return 1.0;
    return (r > 1.0) ? (1.0 / r) : r;
}

// ----------------------------------------------------------------------------
// Texto tematico das costas (cada letra recebe brilho, contorno e efeitos do boss)
// ----------------------------------------------------------------------------
/**
 * @brief Desenha um texto letra a letra com o estilo do chefe da carta.
 *
 * Cada letra recebe brilho, contorno, ondulação e, na carta da Malenia,
 * "pingos" escorrendo.
 *
 * @param idx   Índice da carta (0 a 4).
 * @param texto Texto (até 31 letras).
 * @param cx    Centro X do texto.
 * @param y     Y do texto.
 * @param fonte Tamanho da fonte.
 */
static void cartasTextoTematico(int idx, const char *texto, float cx, float y, float fonte)
{
    const EstiloCarta *e = &estilosCartas[idx];
    Font f = temFonteCarta[idx] ? fontesCartas[idx] : GetFontDefault();
    int n = (int)strlen(texto);
    if (n > 31) // limite do vetor larg[32]
        n = 31;

    float larg[32];
    float total = 0.0f;
    for (int i = 0; i < n; i++)
    {
        char s[2] = {texto[i], '\0'};
        larg[i] = MeasureTextEx(f, s, fonte, 0.0f).x;
        total += larg[i] + e->espacamento;
    }
    total -= e->espacamento;

    float x = cx - total / 2.0f;
    for (int i = 0; i < n; i++)
    {
        char s[2] = {texto[i], '\0'};
        float yy = y + sinf(i * 1.1f) * e->ondulacao;
        Vector2 p = {x, yy};

        // brilho suave
        for (int g = 0; g < 8; g++)
        {
            float a = g * (PI / 4.0f);
            DrawTextEx(f, s, (Vector2){p.x + cosf(a) * 6.0f, p.y + sinf(a) * 6.0f}, fonte, 0.0f, Fade(e->brilho, 0.20f));
        }
        // contorno duplo (dourado) opcional
        if (e->contornoDuplo)
        {
            for (int g = 0; g < 8; g++)
            {
                float a = g * (PI / 4.0f);
                DrawTextEx(f, s, (Vector2){p.x + cosf(a) * 4.5f, p.y + sinf(a) * 4.5f}, fonte, 0.0f, e->moldura);
            }
        }
        // contorno escuro
        for (int g = 0; g < 8; g++)
        {
            float a = g * (PI / 4.0f);
            DrawTextEx(f, s, (Vector2){p.x + cosf(a) * 2.5f, p.y + sinf(a) * 2.5f}, fonte, 0.0f, e->contorno);
        }
        DrawTextEx(f, s, p, fonte, 0.0f, e->texto);

        // pingos de podridao escorrendo das letras
        if (e->pingos && (i % 2 == 0) && texto[i] != ' ')
        {
            float px = x + larg[i] * 0.5f;
            float py = yy + fonte * 0.92f;
            float comp = 8.0f + (float)((i * 7) % 16);
            DrawRectangle((int)px - 1, (int)py, 3, (int)comp, e->brilho);
            DrawCircleV((Vector2){px, py + comp}, 3.6f, e->brilho);
        }
        x += larg[i] + e->espacamento;
    }
}

/**
 * @brief Escreve a dificuldade nas costas da carta.
 *
 * "MUITO FACIL" vira duas linhas: MUITO (pequeno) e FACIL (grande).
 *
 * @param idx Índice da carta (0 a 4).
 * @param cx  Centro X.
 * @param y   Y da palavra principal.
 */
static void cartasRotuloTematico(int idx, float cx, float y)
{
    const char *rotulo = rotulosDificuldade[idx];
    const char *esp = strchr(rotulo, ' ');
    float fonte = (float)estilosCartas[idx].fonte;

    if (esp != NULL)
    {
        char pre[16];
        int n = (int)(esp - rotulo);
        if (n > 15)
            n = 15;
        memcpy(pre, rotulo, (size_t)n);
        pre[n] = '\0';
        cartasTextoTematico(idx, pre, cx, y - fonte * 0.62f - 6.0f, fonte * 0.55f);
        cartasTextoTematico(idx, esp + 1, cx, y, fonte);
    }
    else
    {
        cartasTextoTematico(idx, rotulo, cx, y, fonte);
    }
}

// ----------------------------------------------------------------------------
// Artes procedurais (silhuetas simples inspiradas em cada boss)
// Para usar arte propria, coloque assets/cartas/<boss>.png (veja cartasInicializar)
// ----------------------------------------------------------------------------
/**
 * @brief Desenha a arte procedural da carta I (Margit: ferro e ouro).
 * @note Deve ser chamada dentro de BeginTextureMode (ver cartasCriarCostas).
 */
static void cartaArteMargit(void)
{
    const float cx = CARTA_TEX_L * 0.5f, cy = 205.0f;
    Color ouro = {214, 178, 96, 255};
    Color chifre = {154, 142, 120, 255};

    DrawCircleGradient((int)cx, (int)cy, 150.0f, (Color){214, 178, 96, 70}, (Color){214, 178, 96, 0});
    DrawRing((Vector2){cx, cy - 10.0f}, 112.0f, 117.0f, 0.0f, 360.0f, 72, Fade(ouro, 0.55f));
    DrawRing((Vector2){cx, cy - 10.0f}, 96.0f, 98.0f, 0.0f, 360.0f, 72, Fade(ouro, 0.35f));
    for (int i = 0; i < 12; i++)
    {
        float a = i * 30.0f * DEG2RAD;
        DrawLineEx((Vector2){cx + cosf(a) * 122.0f, cy - 10.0f + sinf(a) * 122.0f},
                   (Vector2){cx + cosf(a) * 146.0f, cy - 10.0f + sinf(a) * 146.0f}, 2.0f, Fade(ouro, 0.35f));
    }

    // cajado dourado
    DrawLineEx((Vector2){cx + 108.0f, 345.0f}, (Vector2){cx + 74.0f, 150.0f}, 9.0f, ouro);
    DrawRing((Vector2){cx + 72.0f, 140.0f}, 9.0f, 15.0f, 0.0f, 360.0f, 24, ouro);

    // capa e ombros
    cartasTri((Vector2){cx, cy + 20.0f}, (Vector2){cx - 128.0f, 350.0f}, (Vector2){cx + 128.0f, 350.0f}, (Color){24, 22, 19, 255});
    cartasTri((Vector2){cx, cy + 50.0f}, (Vector2){cx - 70.0f, 350.0f}, (Vector2){cx + 70.0f, 350.0f}, (Color){38, 34, 29, 255});

    // chifres (saem dos lados da cabeca e curvam para fora)
    cartasArcoAfilado(cx - 110.0f, cy - 70.0f, 48.0f, 0.0f, -110.0f, 18.0f, 3.0f, chifre);
    cartasArcoAfilado(cx + 110.0f, cy - 70.0f, 48.0f, 180.0f, 290.0f, 18.0f, 3.0f, chifre);

    // capuz e rosto sombrio
    DrawEllipse((int)cx, (int)(cy - 18.0f), 70.0f, 80.0f, (Color){10, 9, 8, 255});
    DrawEllipse((int)cx, (int)(cy - 6.0f), 40.0f, 52.0f, (Color){3, 3, 3, 255});
    DrawEllipse((int)(cx - 17.0f), (int)(cy - 14.0f), 9.0f, 3.5f, (Color){255, 226, 140, 255});
    DrawEllipse((int)(cx + 17.0f), (int)(cy - 14.0f), 9.0f, 3.5f, (Color){255, 226, 140, 255});
}

/**
 * @brief Desenha a arte procedural da carta II (Rennala: noite e lua cheia).
 * @note Deve ser chamada dentro de BeginTextureMode (ver cartasCriarCostas).
 */
static void cartaArteRennala(void)
{
    const float cx = CARTA_TEX_L * 0.5f, cy = 200.0f;

    cartasEstrelas(3, 60, (Color){220, 232, 255, 255}, 330.0f);
    DrawCircleGradient((int)cx, (int)(cy - 30.0f), 150.0f, (Color){170, 200, 255, 110}, (Color){120, 160, 255, 0});

    // lua cheia
    DrawCircle((int)cx, (int)(cy - 30.0f), 78.0f, (Color){232, 240, 255, 255});
    DrawCircle((int)(cx - 26.0f), (int)(cy - 52.0f), 15.0f, (Color){206, 220, 246, 255});
    DrawCircle((int)(cx + 30.0f), (int)(cy - 20.0f), 11.0f, (Color){210, 224, 248, 255});
    DrawCircle((int)(cx - 8.0f), (int)(cy + 4.0f), 8.0f, (Color){214, 226, 250, 255});
    DrawRing((Vector2){cx, cy - 30.0f}, 86.0f, 89.0f, 0.0f, 360.0f, 64, Fade((Color){190, 214, 255, 255}, 0.5f));

    // vestes
    cartasTri((Vector2){cx, cy + 20.0f}, (Vector2){cx - 104.0f, 350.0f}, (Vector2){cx + 104.0f, 350.0f}, (Color){12, 16, 50, 255});
    cartasTri((Vector2){cx, cy + 50.0f}, (Vector2){cx - 56.0f, 350.0f}, (Vector2){cx + 56.0f, 350.0f}, (Color){26, 34, 86, 255});
    DrawEllipse((int)cx, (int)(cy + 34.0f), 30.0f, 36.0f, (Color){8, 10, 34, 255});

    // coroa de pontas
    for (int i = -2; i <= 2; i++)
    {
        float bx = cx + i * 13.0f;
        float alt = (i == 0) ? 40.0f : ((i % 2 == 0) ? 30.0f : 22.0f);
        cartasTri((Vector2){bx, cy - 6.0f - alt}, (Vector2){bx - 7.0f, cy + 8.0f}, (Vector2){bx + 7.0f, cy + 8.0f}, (Color){206, 222, 255, 255});
    }

    // livro aberto
    cartasQuad((Vector2){cx - 76.0f, 292.0f}, (Vector2){cx - 76.0f, 338.0f}, (Vector2){cx, 352.0f}, (Vector2){cx, 306.0f}, (Color){222, 228, 244, 255});
    cartasQuad((Vector2){cx + 76.0f, 292.0f}, (Vector2){cx, 306.0f}, (Vector2){cx, 352.0f}, (Vector2){cx + 76.0f, 338.0f}, (Color){206, 214, 238, 255});
    DrawLineEx((Vector2){cx, 306.0f}, (Vector2){cx, 352.0f}, 2.0f, (Color){70, 90, 160, 255});

    // cajado com orbe
    DrawLineEx((Vector2){cx + 92.0f, 352.0f}, (Vector2){cx + 92.0f, 176.0f}, 6.0f, (Color){150, 172, 224, 255});
    DrawCircleGradient((int)(cx + 92.0f), 164, 30.0f, (Color){210, 232, 255, 255}, (Color){120, 170, 255, 0});
    DrawCircle((int)(cx + 92.0f), 164, 9.0f, WHITE);
}

/**
 * @brief Desenha a arte procedural da carta III (Radahn: estrelas e brasa).
 * @note Deve ser chamada dentro de BeginTextureMode (ver cartasCriarCostas).
 */
static void cartaArteRadahn(void)
{
    const float cx = CARTA_TEX_L * 0.5f, cy = 200.0f;
    Color laranja = {255, 144, 44, 255};
    Color brasa = {255, 92, 32, 255};
    Vector2 centro = {cx, cy - 20.0f};

    DrawCircleGradient((int)centro.x, (int)centro.y, 165.0f, (Color){255, 120, 30, 120}, (Color){255, 60, 0, 0});
    cartasEstrelas(11, 46, (Color){255, 222, 172, 255}, 330.0f);

    // estrela de gravidade (16 raios)
    for (int i = 0; i < 16; i++)
    {
        float a = i * 22.5f * DEG2RAD;
        float comp = (i % 2 == 0) ? 150.0f : 104.0f;
        float da = 0.11f;
        Vector2 ponta = {centro.x + cosf(a) * comp, centro.y + sinf(a) * comp};
        Vector2 b1 = {centro.x + cosf(a - da) * 56.0f, centro.y + sinf(a - da) * 56.0f};
        Vector2 b2 = {centro.x + cosf(a + da) * 56.0f, centro.y + sinf(a + da) * 56.0f};
        cartasTri(ponta, b1, b2, Fade(laranja, (i % 2 == 0) ? 0.55f : 0.35f));
    }
    DrawRing(centro, 56.0f, 62.0f, 0.0f, 360.0f, 64, Fade(laranja, 0.85f));

    // espirais de gravidade
    cartasArcoAfilado(centro.x, centro.y, 92.0f, 20.0f, 150.0f, 1.0f, 5.0f, Fade(brasa, 0.6f));
    cartasArcoAfilado(centro.x, centro.y, 118.0f, 200.0f, 330.0f, 1.0f, 5.0f, Fade(brasa, 0.6f));
    cartasArcoAfilado(centro.x, centro.y, 76.0f, 100.0f, 250.0f, 1.0f, 4.0f, Fade(laranja, 0.5f));

    // duas espadas gigantes cruzadas
    DrawLineEx((Vector2){cx - 122.0f, 348.0f}, (Vector2){cx + 96.0f, 118.0f}, 11.0f, (Color){86, 48, 34, 255});
    DrawLineEx((Vector2){cx + 122.0f, 348.0f}, (Vector2){cx - 96.0f, 118.0f}, 11.0f, (Color){86, 48, 34, 255});
    DrawLineEx((Vector2){cx - 122.0f, 348.0f}, (Vector2){cx + 96.0f, 118.0f}, 3.0f, (Color){255, 190, 120, 255});
    DrawLineEx((Vector2){cx + 122.0f, 348.0f}, (Vector2){cx - 96.0f, 118.0f}, 3.0f, (Color){255, 190, 120, 255});

    // corpo, ombreiras e elmo
    cartasTri((Vector2){cx, cy + 20.0f}, (Vector2){cx - 126.0f, 350.0f}, (Vector2){cx + 126.0f, 350.0f}, (Color){30, 12, 8, 255});
    DrawCircle((int)(cx - 88.0f), 298, 38.0f, (Color){46, 20, 12, 255});
    DrawCircle((int)(cx + 88.0f), 298, 38.0f, (Color){46, 20, 12, 255});
    DrawEllipse((int)cx, (int)(cy - 4.0f), 50.0f, 62.0f, (Color){40, 18, 10, 255});
    cartasTri((Vector2){cx, cy - 134.0f}, (Vector2){cx - 20.0f, cy - 50.0f}, (Vector2){cx + 20.0f, cy - 50.0f}, (Color){90, 34, 18, 255});

    // chifres em crescente
    cartasArcoAfilado(cx - 100.0f, cy - 50.0f, 56.0f, 10.0f, -100.0f, 16.0f, 3.0f, (Color){128, 60, 32, 255});
    cartasArcoAfilado(cx + 100.0f, cy - 50.0f, 56.0f, 170.0f, 280.0f, 16.0f, 3.0f, (Color){128, 60, 32, 255});

    // viseira em brasa
    DrawRectangle((int)(cx - 28.0f), (int)(cy - 12.0f), 56, 8, (Color){255, 170, 70, 255});
}

/**
 * @brief Desenha a arte procedural da carta IV (Radagon: dourado).
 * @note Deve ser chamada dentro de BeginTextureMode (ver cartasCriarCostas).
 */
static void cartaArteRadagon(void)
{
    const float cx = CARTA_TEX_L * 0.5f, cy = 205.0f;
    Color ouro = {255, 216, 92, 255};
    Color ouroEscuro = {156, 110, 26, 255};
    Vector2 centro = {cx, cy - 20.0f};

    DrawCircleGradient((int)centro.x, (int)centro.y, 160.0f, (Color){255, 216, 92, 120}, (Color){255, 200, 60, 0});

    // raios da Ordem Dourada
    for (int i = 0; i < 24; i++)
    {
        float a = i * 15.0f * DEG2RAD;
        float comp = (i % 2 == 0) ? 160.0f : 118.0f;
        float da = 0.05f;
        Vector2 ponta = {centro.x + cosf(a) * comp, centro.y + sinf(a) * comp};
        Vector2 b1 = {centro.x + cosf(a - da) * 70.0f, centro.y + sinf(a - da) * 70.0f};
        Vector2 b2 = {centro.x + cosf(a + da) * 70.0f, centro.y + sinf(a + da) * 70.0f};
        cartasTri(ponta, b1, b2, Fade(ouro, 0.45f));
    }

    // anel quebrado, como o Anel Pretérito
    for (int i = 0; i < 6; i++)
    {
        cartasArcoAfilado(centro.x, centro.y, 98.0f, i * 60.0f + 6.0f, i * 60.0f + 50.0f, 5.0f, 5.0f, ouro);
    }
    DrawRing(centro, 128.0f, 131.0f, 0.0f, 360.0f, 72, Fade(ouro, 0.5f));

    // martelo de Marika
    DrawLineEx((Vector2){cx + 98.0f, 350.0f}, (Vector2){cx + 98.0f, 178.0f}, 10.0f, ouroEscuro);
    DrawRectangleRounded((Rectangle){cx + 60.0f, 138.0f, 76.0f, 50.0f}, 0.25f, 6, ouro);
    DrawRectangleRoundedLines((Rectangle){cx + 60.0f, 138.0f, 76.0f, 50.0f}, 0.25f, 6, ouroEscuro);

    // cabelo ruivo ao vento
    cartasTri((Vector2){cx - 20.0f, cy - 62.0f}, (Vector2){cx - 100.0f, 338.0f}, (Vector2){cx - 30.0f, 338.0f}, (Color){176, 56, 24, 255});
    cartasTri((Vector2){cx + 20.0f, cy - 62.0f}, (Vector2){cx + 30.0f, 338.0f}, (Vector2){cx + 70.0f, 338.0f}, (Color){176, 56, 24, 255});

    // armadura
    cartasTri((Vector2){cx, cy + 25.0f}, (Vector2){cx - 122.0f, 350.0f}, (Vector2){cx + 122.0f, 350.0f}, (Color){70, 48, 12, 255});
    DrawLineEx((Vector2){cx, cy + 25.0f}, (Vector2){cx - 122.0f, 350.0f}, 3.0f, ouro);
    DrawLineEx((Vector2){cx, cy + 25.0f}, (Vector2){cx + 122.0f, 350.0f}, 3.0f, ouro);

    // elmo dourado com viseira em cruz
    DrawEllipse((int)cx, (int)(cy - 20.0f), 50.0f, 62.0f, ouroEscuro);
    DrawEllipse((int)(cx - 14.0f), (int)(cy - 36.0f), 24.0f, 32.0f, Fade(ouro, 0.40f));
    DrawRectangle((int)(cx - 34.0f), (int)(cy - 30.0f), 68, 8, (Color){22, 12, 2, 255});
    DrawRectangle((int)(cx - 3.0f), (int)(cy - 30.0f), 6, 46, (Color){22, 12, 2, 255});
    for (int i = -1; i <= 1; i++)
    {
        float bx = cx + i * 22.0f;
        cartasTri((Vector2){bx, cy - 100.0f}, (Vector2){bx - 9.0f, cy - 70.0f}, (Vector2){bx + 9.0f, cy - 70.0f}, ouro);
    }
}

/**
 * @brief Desenha a arte procedural da carta V (Malenia: podridão escarlate).
 * @note Deve ser chamada dentro de BeginTextureMode (ver cartasCriarCostas).
 */
static void cartaArteMalenia(void)
{
    const float cx = CARTA_TEX_L * 0.5f, cy = 205.0f;
    Color rubi = {232, 52, 42, 255};
    Color rubiEscuro = {124, 12, 14, 255};
    Color ouro = {216, 150, 70, 255};
    Vector2 centro = {cx, cy - 10.0f};

    DrawCircleGradient((int)centro.x, (int)centro.y, 170.0f, (Color){255, 50, 40, 130}, (Color){200, 0, 0, 0});

    // flor da Podridao Escarlate
    for (int i = 0; i < 14; i++)
    {
        float a = (i * (360.0f / 14.0f) + 8.0f) * DEG2RAD;
        float da = 0.17f;
        Vector2 ponta = {centro.x + cosf(a) * 158.0f, centro.y + sinf(a) * 158.0f};
        Vector2 b1 = {centro.x + cosf(a - da) * 40.0f, centro.y + sinf(a - da) * 40.0f};
        Vector2 b2 = {centro.x + cosf(a + da) * 40.0f, centro.y + sinf(a + da) * 40.0f};
        cartasTri(ponta, b1, b2, Fade((i % 2 == 0) ? rubi : rubiEscuro, 0.55f));
        DrawCircleV(ponta, 4.0f, Fade((Color){255, 160, 90, 255}, 0.85f));
    }
    // esporos
    for (int i = 0; i < 34; i++)
    {
        float x = 26.0f + cartasRuido(500 + i * 2) * (CARTA_TEX_L - 52.0f);
        float y = 26.0f + cartasRuido(501 + i * 2) * 300.0f;
        DrawCircleV((Vector2){x, y}, 1.0f + cartasRuido(700 + i) * 2.4f, Fade((Color){255, 140, 80, 255}, 0.55f));
    }

    // cabelo ruivo escorrendo
    cartasTri((Vector2){cx - 6.0f, cy - 40.0f}, (Vector2){cx - 96.0f, 350.0f}, (Vector2){cx - 20.0f, 350.0f}, (Color){176, 30, 18, 255});
    cartasTri((Vector2){cx + 6.0f, cy - 40.0f}, (Vector2){cx + 20.0f, 350.0f}, (Vector2){cx + 96.0f, 350.0f}, (Color){176, 30, 18, 255});

    // corpo
    cartasTri((Vector2){cx, cy + 25.0f}, (Vector2){cx - 92.0f, 350.0f}, (Vector2){cx + 92.0f, 350.0f}, (Color){42, 6, 8, 255});
    DrawCircleV((Vector2){cx - 40.0f, 300.0f}, 6.0f, Fade(rubi, 0.8f));
    DrawCircleV((Vector2){cx + 34.0f, 318.0f}, 8.0f, Fade(rubi, 0.8f));
    DrawCircleV((Vector2){cx + 10.0f, 286.0f}, 4.0f, Fade(rubi, 0.8f));

    // asas do elmo (tres penas de cada lado)
    for (int k = 0; k < 3; k++)
    {
        Color corPena = (k == 1) ? (Color){240, 176, 92, 255} : ouro;
        cartasTri((Vector2){cx - 158.0f + k * 14.0f, cy - 124.0f + k * 34.0f}, (Vector2){cx - 38.0f, cy - 64.0f + k * 16.0f}, (Vector2){cx - 38.0f, cy - 30.0f + k * 16.0f}, corPena);
        cartasTri((Vector2){cx + 158.0f - k * 14.0f, cy - 124.0f + k * 34.0f}, (Vector2){cx + 38.0f, cy - 64.0f + k * 16.0f}, (Vector2){cx + 38.0f, cy - 30.0f + k * 16.0f}, corPena);
    }

    // elmo
    DrawEllipse((int)cx, (int)(cy - 10.0f), 42.0f, 54.0f, (Color){170, 112, 50, 255});
    DrawEllipse((int)cx, (int)(cy - 2.0f), 28.0f, 36.0f, (Color){22, 4, 4, 255});

    // katana
    DrawLineEx((Vector2){cx + 120.0f, 348.0f}, (Vector2){cx + 8.0f, 92.0f}, 5.0f, (Color){236, 226, 226, 255});
    DrawLineEx((Vector2){cx + 120.0f, 348.0f}, (Vector2){cx + 8.0f, 92.0f}, 1.5f, WHITE);
}

static void (*const artesProcedurais[CARTAS_QTD])(void) = {cartaArteMargit, cartaArteRennala, cartaArteRadahn, cartaArteRadagon, cartaArteMalenia};

// ----------------------------------------------------------------------------
// Monta a textura das costas da carta (uma vez, na inicializacao)
// ----------------------------------------------------------------------------
/**
 * @brief Desenha uma vez as costas de uma carta em uma RenderTexture.
 *
 * Usa a arte de assets/cartas/<boss>.png se existir; senão, a arte procedural.
 *
 * @param idx Índice da carta (0 a 4).
 */
static void cartasCriarCostas(int idx)
{
    const EstiloCarta *e = &estilosCartas[idx];
    const float W = (float)CARTA_TEX_L;
    const float H = (float)CARTA_TEX_A;
    Rectangle arte = {16.0f, 16.0f, W - 32.0f, H - 32.0f};

    BeginTextureMode(costasCartas[idx]);
    ClearBackground(BLANK);

    DrawRectangleRounded((Rectangle){0.0f, 0.0f, W, H}, 0.06f, 12, e->moldura);
    DrawRectangleRounded((Rectangle){5.0f, 5.0f, W - 10.0f, H - 10.0f}, 0.05f, 12, (Color){12, 10, 8, 255});
    DrawRectangleGradientV((int)arte.x, (int)arte.y, (int)arte.width, (int)arte.height, e->fundoTopo, e->fundoBase);

    if (temArteCarta[idx])
    {
        // arte propria do usuario: preenche a moldura (corte central, sem distorcer)
        Texture2D t = artesCartas[idx];
        float proporcaoAlvo = arte.width / arte.height;
        float proporcaoImg = (float)t.width / (float)t.height;
        Rectangle src = {0.0f, 0.0f, (float)t.width, (float)t.height};
        if (proporcaoImg > proporcaoAlvo)
        {
            src.width = t.height * proporcaoAlvo;
            src.x = ((float)t.width - src.width) * 0.5f;
        }
        else
        {
            src.height = t.width / proporcaoAlvo;
            src.y = ((float)t.height - src.height) * 0.5f;
        }
        DrawTexturePro(t, src, arte, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
    }
    else
    {
        artesProcedurais[idx]();
    }

    // faixa escura para dar leitura ao texto por cima da imagem
    DrawRectangleGradientV((int)arte.x, 290, (int)arte.width, (int)(arte.y + arte.height - 290.0f), (Color){0, 0, 0, 0}, (Color){0, 0, 0, 215});

    // moldura interna e ornamentos nos cantos
    DrawRectangleLinesEx(arte, 3.0f, e->molduraInterna);
    DrawRectangleLinesEx((Rectangle){arte.x + 8.0f, arte.y + 8.0f, arte.width - 16.0f, arte.height - 16.0f}, 1.0f, Fade(e->moldura, 0.5f));
    DrawPoly((Vector2){arte.x, arte.y}, 4, 10.0f, 0.0f, e->moldura);
    DrawPoly((Vector2){arte.x + arte.width, arte.y}, 4, 10.0f, 0.0f, e->moldura);
    DrawPoly((Vector2){arte.x, arte.y + arte.height}, 4, 10.0f, 0.0f, e->moldura);
    DrawPoly((Vector2){arte.x + arte.width, arte.y + arte.height}, 4, 10.0f, 0.0f, e->moldura);

    // medalhao com o numeral da dificuldade
    DrawCircleV((Vector2){W / 2.0f, 46.0f}, 23.0f, (Color){8, 6, 6, 215});
    DrawRing((Vector2){W / 2.0f, 46.0f}, 21.0f, 24.0f, 0.0f, 360.0f, 36, e->moldura);
    DrawText(numeraisCarta[idx], (int)(W / 2.0f - MeasureText(numeraisCarta[idx], 24) / 2.0f), 34, 24, e->texto);

    // dificuldade escrita por cima da imagem, no estilo do boss
    cartasRotuloTematico(idx, W / 2.0f, H - 150.0f);
    DrawLineEx((Vector2){W / 2.0f - 90.0f, H - 84.0f}, (Vector2){W / 2.0f + 90.0f, H - 84.0f}, 2.0f, Fade(e->moldura, 0.8f));
    DrawPoly((Vector2){W / 2.0f, H - 84.0f}, 4, 6.0f, 0.0f, e->moldura);
    cartasTextoTematico(idx, nomesBossCarta[idx], W / 2.0f, H - 68.0f, 22.0f);

    EndTextureMode();
    SetTextureFilter(costasCartas[idx].texture, TEXTURE_FILTER_BILINEAR);
    temCostasCartas[idx] = 1;
}

// ----------------------------------------------------------------------------
// Ciclo de vida
// ----------------------------------------------------------------------------
/**
 * @brief Prepara o sistema de cartas: converte o banco em cartas e gera as texturas.
 *
 * Precisa ser chamada depois de InitWindow (as texturas dependem da janela).
 * Com o banco vazio, o sistema de cartas fica desativado.
 *
 * @param banco Banco de perguntas.
 */
static void cartasInicializar(Banco *banco)
{
    memset(&estadoCartas, 0, sizeof(estadoCartas));
    estadoCartas.hover = -1;
    estadoCartas.escolhida = -1;

    if (banco == NULL || banco->quantidade <= 0)
    {
        TraceLog(LOG_WARNING, "CARTAS: banco de perguntas vazio, sistema de cartas desativado");
        return;
    }

    totalCartas = banco->quantidade;
    todasCartas = malloc((size_t)totalCartas * sizeof(CardGame));
    filtroCartas = malloc((size_t)totalCartas * sizeof(CardGame));
    if (todasCartas == NULL || filtroCartas == NULL)
    {
        liberarCards(todasCartas);
        liberarCards(filtroCartas);
        todasCartas = NULL;
        filtroCartas = NULL;
        totalCartas = 0;
        return;
    }
    inicializarCards(todasCartas, totalCartas, banco);

    for (int i = 0; i < CARTAS_QTD; i++)
    {
        // arte opcional: assets/cartas/margit.png, rennala.png, radahn.png, radagon.png, malenia.png
        const char *caminhoArte = TextFormat("assets/cartas/%s.png", arquivosBossCarta[i]);
        if (FileExists(caminhoArte))
        {
            artesCartas[i] = LoadTexture(caminhoArte);
            temArteCarta[i] = (artesCartas[i].id > 0);
        }

        // fonte opcional: assets/fonts/margit.ttf, rennala.ttf, ...
        const char *caminhoFonte = TextFormat("assets/fonts/%s.ttf", arquivosBossCarta[i]);
        if (FileExists(caminhoFonte))
        {
            fontesCartas[i] = LoadFontEx(caminhoFonte, 96, NULL, 0);
            temFonteCarta[i] = (fontesCartas[i].texture.id > 0);
            if (temFonteCarta[i])
                SetTextureFilter(fontesCartas[i].texture, TEXTURE_FILTER_BILINEAR);
        }

        costasCartas[i] = LoadRenderTexture(CARTA_TEX_L, CARTA_TEX_A);
        cartasCriarCostas(i);
    }
}

/**
 * @brief Libera as texturas, fontes e vetores do sistema de cartas.
 */
static void cartasFinalizar(void)
{
    for (int i = 0; i < CARTAS_QTD; i++)
    {
        if (temCostasCartas[i])
            UnloadRenderTexture(costasCartas[i]);
        if (temArteCarta[i])
            UnloadTexture(artesCartas[i]);
        if (temFonteCarta[i])
            UnloadFont(fontesCartas[i]);
        temCostasCartas[i] = 0;
        temArteCarta[i] = 0;
        temFonteCarta[i] = 0;
    }
    liberarCards(todasCartas);
    liberarCards(filtroCartas);
    todasCartas = NULL;
    filtroCartas = NULL;
    totalCartas = 0;
}

/**
 * @brief Diz se a tela de cartas está aberta.
 * @return 1 se alguma fase de cartas está em andamento, 0 se não.
 */
static int cartasAtivo(void)
{
    return estadoCartas.fase != CARTAS_FASE_INATIVA;
}

/**
 * @brief Troca a fase da tela de cartas e zera o cronômetro da fase.
 * @param fase Nova fase.
 */
static void cartasMudarFase(FaseCartas fase)
{
    estadoCartas.fase = fase;
    estadoCartas.tempoFase = 0.0f;
}

/**
 * @brief Abre a escolha de cartas depois de o herói vencer um combate.
 *
 * Marca como indisponível a carta de um nível sem nenhuma pergunta no banco;
 * se nenhum nível tiver perguntas, não abre.
 */
static void cartasAgendarEscolha(void)
{
    if (todasCartas == NULL || filtroCartas == NULL || totalCartas <= 0 || cartasAtivo())
        return;

    int algumaDisponivel = 0;
    for (int i = 0; i < CARTAS_QTD; i++)
    {
        estadoCartas.disponivel[i] = (ProcurarDificuldadeCards(todasCartas, totalCartas, ordemDificuldades[i], filtroCartas) > 0);
        estadoCartas.elevacao[i] = 0.0f;
        if (estadoCartas.disponivel[i])
            algumaDisponivel = 1;
    }
    if (!algumaDisponivel)
        return;

    estadoCartas.hover = -1;
    estadoCartas.escolhida = -1;
    cartasMudarFase(CARTAS_FASE_ESPERA);
}

// ----------------------------------------------------------------------------
// Layout
// ----------------------------------------------------------------------------
/**
 * @brief Calcula onde fica cada uma das 5 cartas na mesa.
 * @param i        Índice da carta (0 a 4).
 * @param largTela Largura da tela.
 * @param altTela  Altura da tela.
 * @return Retângulo da carta.
 */
static Rectangle cartasRetanguloEscolha(int i, float largTela, float altTela)
{
    float cw = fminf(altTela * 0.50f * ((float)CARTA_TEX_L / (float)CARTA_TEX_A), largTela * 0.15f);
    float ch = cw * ((float)CARTA_TEX_A / (float)CARTA_TEX_L);
    float gap = cw * 0.22f;
    float total = CARTAS_QTD * cw + (CARTAS_QTD - 1) * gap;
    float x0 = (largTela - total) / 2.0f;
    return (Rectangle){x0 + i * (cw + gap), altTela * 0.54f - ch / 2.0f, cw, ch};
}

/**
 * @brief Calcula o painel da pergunta conforme a tela e o tamanho do enunciado.
 * @param largTela Largura da tela.
 * @param altTela  Altura da tela.
 * @return Posições do painel, do enunciado e dos botões.
 */
static LayoutPergunta cartasLayoutPergunta(float largTela, float altTela)
{
    LayoutPergunta l;
    const float margem = 44.0f, cabecalho = 84.0f, rodape = 96.0f, bh = 72.0f, gap = 14.0f;
    float pw = fminf(largTela * 0.82f, 1200.0f);

    // Mede (sem desenhar) a altura do enunciado para dimensionar o painel conforme o texto
    l.alturaEnunciado = (float)cartasTextoQuebrado(estadoCartas.carta.enunciado, 0.0f, 0.0f, pw - margem * 2.0f, 30, WHITE, 0, 0);
    float ph = 24.0f + cabecalho + l.alturaEnunciado + 26.0f + 4.0f * bh + 3.0f * gap + rodape;
    float px = (largTela - pw) / 2.0f;
    float py = (altTela - ph) / 2.0f;
    if (py < 8.0f)
        py = 8.0f;

    l.painel = (Rectangle){px, py, pw, ph};
    l.yCabecalho = py + 24.0f;
    l.yEnunciado = l.yCabecalho + cabecalho;
    float yb = l.yEnunciado + l.alturaEnunciado + 26.0f;
    for (int j = 0; j < 4; j++)
    {
        l.botoes[j] = (Rectangle){px + margem, yb + j * (bh + gap), pw - margem * 2.0f, bh};
    }
    l.yRodape = yb + 4.0f * bh + 3.0f * gap + 16.0f;
    return l;
}

// ----------------------------------------------------------------------------
// Logica
// ----------------------------------------------------------------------------
/**
 * @brief Sorteia uma pergunta do nível da carta escolhida e inicia a revelação.
 * @param idx Índice da carta escolhida (0 a 4).
 */
static void cartasSortearPergunta(int idx)
{
    EstadoCartas *e = &estadoCartas;
    int n = ProcurarDificuldadeCards(todasCartas, totalCartas, ordemDificuldades[idx], filtroCartas);
    if (n <= 0)
        return;

    // EscolherCard copia a carta sorteada para e->carta, então filtroCartas pode ser reutilizado depois
    EscolherCard(filtroCartas, &e->carta, n);
    e->escolhida = idx;
    e->hover = -1;
    cartasMudarFase(CARTAS_FASE_REVELANDO);
}

/**
 * @brief Confere a resposta e aplica o multiplicador (acerto) ou o redutor (erro).
 *
 * O poder do herói nunca fica abaixo de 1.
 *
 * @param arena  Arena (o poder do herói é alterado).
 * @param indice Alternativa escolhida (0 = A ... 3 = D).
 */
static void cartasResponder(Arena *arena, int indice)
{
    EstadoCartas *e = &estadoCartas;
    Heroi *heroi = &arena->heroi;

    e->letraEscolhida = (char)('A' + indice);
    e->acertou = (toupper((unsigned char)e->letraEscolhida) == toupper((unsigned char)e->carta.correta));
    e->poderAntes = heroi->poder;

    // Acerto multiplica o poder; erro reduz (fator < 1). O poder nunca fica abaixo de 1.
    double fator = e->acertou ? cartasFatorAcerto(&e->carta) : cartasFatorErro(&e->carta);
    double novo = (double)heroi->poder * fator;
    heroi->poder = clampIntMax((long long)novo, 1, PODER_MAX_INIMIGO * 2);
    e->poderDepois = heroi->poder;

    if (e->acertou)
    {
        heroi->timerLevelUp = 0.85f;
        adicionarPopup(TextFormat("PODER x%g!", fator),
                       (Vector2){heroi->pos.x, heroi->pos.y - 170.0f},
                       (Color){255, 220, 70, 255});
        atualizarEvolucaoHeroi(arena, 0);
    }
    else
    {
        heroi->timerRecuo = 0.45f;
        adicionarPopup(TextFormat("PODER x%.2f", fator),
                       (Vector2){heroi->pos.x, heroi->pos.y - 170.0f},
                       (Color){255, 75, 75, 255});
    }
    cartasMudarFase(CARTAS_FASE_RESULTADO);
}

/**
 * @brief Trata mouse/teclado e avança as fases da tela de cartas.
 *
 * Os tempos mínimos de cada fase evitam que o mesmo clique que abriu a
 * tela já conte como escolha ou resposta. Atalhos: 1-5 escolhem a carta,
 * A-D respondem, Enter/Espaço fecham o resultado.
 *
 * @param arena    Arena (o poder do herói pode mudar).
 * @param dt       Tempo do quadro em segundos.
 * @param largTela Largura da tela.
 * @param altTela  Altura da tela.
 */
static void cartasAtualizar(Arena *arena, float dt, float largTela, float altTela)
{
    EstadoCartas *e = &estadoCartas;
    e->tempoFase += dt;

    Vector2 mouse = GetMousePosition();
    int clicou = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    // Os tempos mínimos (0.7s, 0.25s, 0.4s) em cada fase evitam que o mesmo clique
    // que abriu/avançou a tela já conte como escolha ou resposta.
    switch (e->fase)
    {
    case CARTAS_FASE_ESPERA:
        if (e->tempoFase >= 0.9f)
            cartasMudarFase(CARTAS_FASE_ESCOLHA);
        break;

    case CARTAS_FASE_ESCOLHA:
    {
        int pronto = (e->tempoFase > 0.7f);
        int escolhido = -1;
        e->hover = -1;

        for (int i = 0; i < CARTAS_QTD; i++)
        {
            Rectangle r = cartasRetanguloEscolha(i, largTela, altTela);
            if (pronto && e->disponivel[i] && CheckCollisionPointRec(mouse, r))
                e->hover = i;
        }
        for (int i = 0; i < CARTAS_QTD; i++)
        {
            float alvo = (e->hover == i) ? 1.0f : 0.0f;
            e->elevacao[i] += (alvo - e->elevacao[i]) * fminf(1.0f, dt * 12.0f);
        }

        if (pronto)
        {
            if (clicou && e->hover >= 0)
                escolhido = e->hover;
            for (int k = 0; k < CARTAS_QTD; k++)
            {
                // KEY_ONE..KEY_FIVE e KEY_A..KEY_D são códigos consecutivos no raylib
                if (IsKeyPressed(KEY_ONE + k) && e->disponivel[k])
                    escolhido = k;
            }
        }
        if (escolhido >= 0)
            cartasSortearPergunta(escolhido);
        break;
    }

    case CARTAS_FASE_REVELANDO:
        if (e->tempoFase >= 1.2f)
            cartasMudarFase(CARTAS_FASE_PERGUNTA);
        break;

    case CARTAS_FASE_PERGUNTA:
    {
        LayoutPergunta l = cartasLayoutPergunta(largTela, altTela);
        int resp = -1;
        e->hover = -1;
        for (int j = 0; j < 4; j++)
        {
            if (CheckCollisionPointRec(mouse, l.botoes[j]))
                e->hover = j;
        }
        if (clicou && e->hover >= 0)
            resp = e->hover;
        for (int j = 0; j < 4; j++)
        {
            if (IsKeyPressed(KEY_A + j))
                resp = j;
        }
        if (e->tempoFase > 0.25f && resp >= 0)
            cartasResponder(arena, resp);
        break;
    }

    case CARTAS_FASE_RESULTADO:
        if (e->tempoFase > 0.4f && (clicou || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)))
            cartasMudarFase(CARTAS_FASE_INATIVA);
        break;

    default:
        break;
    }
}

// ----------------------------------------------------------------------------
// Desenho
// ----------------------------------------------------------------------------
/**
 * @brief Desenha as costas de uma carta.
 * @param idx    Índice da carta (0 a 4).
 * @param centro Centro da carta na tela.
 * @param larg   Largura.
 * @param alt    Altura.
 * @param rot    Rotação em graus.
 * @param alpha  Opacidade (0 a 1).
 * @param brilho Intensidade do brilho em volta (0 a 1).
 */
static void cartasDesenharCostas(int idx, Vector2 centro, float larg, float alt, float rot, float alpha, float brilho)
{
    if (!temCostasCartas[idx] || larg < 1.0f)
        return;

    if (brilho > 0.01f)
    {
        for (int g = 1; g <= 4; g++)
        {
            Rectangle gr = {centro.x - larg / 2.0f - g * 5.0f, centro.y - alt / 2.0f - g * 5.0f, larg + g * 10.0f, alt + g * 10.0f};
            DrawRectangleRounded(gr, 0.08f, 10, Fade(estilosCartas[idx].brilho, 0.10f * brilho * alpha));
        }
    }

    Rectangle src = {0.0f, 0.0f, (float)CARTA_TEX_L, -(float)CARTA_TEX_A}; // altura negativa: RenderTexture vem invertida
    Rectangle dst = {centro.x, centro.y, larg, alt};
    DrawTexturePro(costasCartas[idx].texture, src, dst, (Vector2){larg / 2.0f, alt / 2.0f}, rot, Fade(WHITE, alpha));
}

/**
 * @brief Desenha a frente da carta revelada (quanto vale o acerto e o erro).
 * @param idx          Índice da carta (0 a 4).
 * @param centro       Centro da carta na tela.
 * @param larg         Largura.
 * @param alt          Altura.
 * @param mostrarTexto 0 durante a virada (carta muito estreita para texto).
 */
static void cartasDesenharFrente(int idx, Vector2 centro, float larg, float alt, int mostrarTexto)
{
    const EstiloCarta *est = &estilosCartas[idx];
    Rectangle r = {centro.x - larg / 2.0f, centro.y - alt / 2.0f, larg, alt};

    DrawRectangleRounded(r, 0.06f, 12, est->moldura);
    DrawRectangleRounded((Rectangle){r.x + 5.0f, r.y + 5.0f, r.width - 10.0f, r.height - 10.0f}, 0.05f, 12, (Color){12, 10, 8, 255});
    DrawRectangleGradientV((int)(r.x + 14.0f), (int)(r.y + 14.0f), (int)(r.width - 28.0f), (int)(r.height - 28.0f), est->fundoTopo, est->fundoBase);
    if (!mostrarTexto)
        return;

    int fs = (int)(alt * 0.06f);
    float cx = centro.x;
    const char *txt;
    int y = (int)(r.y + alt * 0.09f);

    txt = nomesBossCarta[idx];
    DrawText(txt, (int)(cx - MeasureText(txt, (int)(fs * 1.3f)) / 2.0f), y, (int)(fs * 1.3f), est->texto);
    txt = rotulosDificuldade[idx];
    DrawText(txt, (int)(cx - MeasureText(txt, fs) / 2.0f), (int)(r.y + alt * 0.19f), fs, est->moldura);
    DrawLineEx((Vector2){cx - larg * 0.3f, r.y + alt * 0.28f}, (Vector2){cx + larg * 0.3f, r.y + alt * 0.28f}, 2.0f, Fade(est->moldura, 0.8f));

    txt = "ACERTO";
    DrawText(txt, (int)(cx - MeasureText(txt, fs) / 2.0f), (int)(r.y + alt * 0.36f), fs, (Color){190, 190, 190, 255});
    txt = TextFormat("x%g", cartasFatorAcerto(&estadoCartas.carta));
    DrawText(txt, (int)(cx - MeasureText(txt, (int)(fs * 2.2f)) / 2.0f), (int)(r.y + alt * 0.43f), (int)(fs * 2.2f), (Color){110, 235, 140, 255});

    txt = "ERRO";
    DrawText(txt, (int)(cx - MeasureText(txt, fs) / 2.0f), (int)(r.y + alt * 0.64f), fs, (Color){190, 190, 190, 255});
    txt = TextFormat("x%.2f", cartasFatorErro(&estadoCartas.carta));
    DrawText(txt, (int)(cx - MeasureText(txt, (int)(fs * 2.0f)) / 2.0f), (int)(r.y + alt * 0.71f), (int)(fs * 2.0f), (Color){255, 100, 90, 255});
}

/**
 * @brief Desenha a mesa com as 5 cartas de costas.
 * @param largTela Largura da tela.
 * @param altTela  Altura da tela.
 */
static void cartasDesenharEscolha(float largTela, float altTela)
{
    const EstadoCartas *e = &estadoCartas;
    float t = e->tempoFase;

    const char *titulo = "ESCOLHA SUA CARTA";
    DrawText(titulo, (int)(largTela / 2.0f - MeasureText(titulo, 46) / 2.0f) + 2, (int)(altTela * 0.07f) + 2, 46, BLACK);
    DrawText(titulo, (int)(largTela / 2.0f - MeasureText(titulo, 46) / 2.0f), (int)(altTela * 0.07f), 46, (Color){255, 215, 80, 255});
    const char *sub = "Quanto mais dificil a pergunta, maior a recompensa. Clique em uma carta ou tecle 1 a 5.";
    DrawText(sub, (int)(largTela / 2.0f - MeasureText(sub, 22) / 2.0f), (int)(altTela * 0.07f) + 62, 22, (Color){210, 205, 195, 255});

    for (int i = 0; i < CARTAS_QTD; i++)
    {
        Rectangle r = cartasRetanguloEscolha(i, largTela, altTela);
        float ti = cartasSuavizar((t - i * 0.09f) / 0.5f);
        float lift = e->elevacao[i];
        float esc = 1.0f + lift * 0.08f;
        float balanco = sinf((float)GetTime() * 1.5f + i) * 1.2f * (1.0f - lift);
        float alpha = ti * (e->disponivel[i] ? 1.0f : 0.35f);

        Vector2 centro = {r.x + r.width / 2.0f,
                          r.y + r.height / 2.0f - lift * r.height * 0.07f - (1.0f - ti) * altTela * 0.7f};
        cartasDesenharCostas(i, centro, r.width * esc, r.height * esc, balanco, alpha, lift);

        // legenda numerica embaixo
        const char *num = TextFormat("%d", i + 1);
        DrawText(num, (int)(r.x + r.width / 2.0f - MeasureText(num, 24) / 2.0f), (int)(r.y + r.height + 22.0f), 24, Fade((Color){230, 220, 190, 255}, ti));
    }
}

/**
 * @brief Desenha a animação da carta escolhida indo ao centro e desvirando.
 * @param largTela Largura da tela.
 * @param altTela  Altura da tela.
 */
static void cartasDesenharRevelando(float largTela, float altTela)
{
    const EstadoCartas *e = &estadoCartas;
    float t = cartasClamp01(e->tempoFase / 1.2f);
    float mover = cartasSuavizar(t / 0.5f);

    for (int i = 0; i < CARTAS_QTD; i++)
    {
        Rectangle r = cartasRetanguloEscolha(i, largTela, altTela);
        Vector2 base = {r.x + r.width / 2.0f, r.y + r.height / 2.0f};

        if (i != e->escolhida)
        {
            float alpha = (1.0f - cartasClamp01(t / 0.4f)) * (e->disponivel[i] ? 1.0f : 0.35f);
            cartasDesenharCostas(i, base, r.width, r.height, 0.0f, alpha, 0.0f);
            continue;
        }

        float alturaFinal = altTela * 0.62f;
        float larguraFinal = alturaFinal * ((float)CARTA_TEX_L / (float)CARTA_TEX_A);
        Vector2 centro = {base.x + (largTela / 2.0f - base.x) * mover, base.y + (altTela / 2.0f - base.y) * mover};
        float larg = r.width + (larguraFinal - r.width) * mover;
        float alt = r.height + (alturaFinal - r.height) * mover;

        float flipT = cartasClamp01((t - 0.3f) / 0.55f);
        float escalaX = fabsf(cosf(flipT * PI));
        if (flipT <= 0.5f)
            cartasDesenharCostas(i, centro, larg * escalaX, alt, 0.0f, 1.0f, mover);
        else
            cartasDesenharFrente(i, centro, larg * escalaX, alt, escalaX > 0.7f);
    }
}

/**
 * @brief Desenha o painel com o enunciado e as alternativas A-D.
 * @param arena     Arena (mostra o poder do herói).
 * @param largTela  Largura da tela.
 * @param altTela   Altura da tela.
 * @param resultado 1 para destacar acerto/erro e mostrar a mudança de poder.
 */
static void cartasDesenharPainel(const Arena *arena, float largTela, float altTela, int resultado)
{
    const EstadoCartas *e = &estadoCartas;
    const EstiloCarta *est = &estilosCartas[e->escolhida];
    LayoutPergunta l = cartasLayoutPergunta(largTela, altTela);
    const float margem = 44.0f;

    DrawRectangleRounded(l.painel, 0.03f, 10, (Color){14, 12, 16, 244});
    DrawRectangleRoundedLines(l.painel, 0.03f, 10, est->moldura);

    // miniatura da carta + nome do boss e dificuldade
    cartasDesenharCostas(e->escolhida, (Vector2){l.painel.x + margem + 30.0f, l.yCabecalho + 38.0f}, 54.0f, 76.0f, 0.0f, 1.0f, 0.0f);
    const char *cab = TextFormat("%s  -  %s", nomesBossCarta[e->escolhida], rotulosDificuldade[e->escolhida]);
    DrawText(cab, (int)(l.painel.x + margem + 76.0f), (int)l.yCabecalho + 4, 34, est->texto);
    const char *regra = TextFormat("Acerto: x%g    Erro: x%.2f", cartasFatorAcerto(&e->carta), cartasFatorErro(&e->carta));
    DrawText(regra, (int)(l.painel.x + margem + 76.0f), (int)l.yCabecalho + 46, 22, (Color){200, 195, 185, 255});

    // enunciado
    cartasTextoQuebrado(e->carta.enunciado, l.painel.x + margem, l.yEnunciado, l.painel.width - margem * 2.0f, 30, (Color){245, 240, 230, 255}, 0, 1);

    // alternativas
    char correta = (char)toupper((unsigned char)e->carta.correta);
    for (int j = 0; j < 4; j++)
    {
        Rectangle b = l.botoes[j];
        char letra = (char)('A' + j);
        Color fundo = (Color){32, 28, 36, 255};
        Color borda = Fade(est->moldura, 0.55f);

        if (!resultado)
        {
            if (e->hover == j)
            {
                fundo = (Color){56, 48, 62, 255};
                borda = est->moldura;
            }
        }
        else if (letra == correta)
        {
            fundo = (Color){28, 104, 58, 255};
            borda = (Color){120, 240, 150, 255};
        }
        else if (letra == e->letraEscolhida)
        {
            fundo = (Color){120, 28, 30, 255};
            borda = (Color){255, 110, 100, 255};
        }
        else
        {
            fundo = (Color){22, 20, 26, 255};
            borda = Fade(est->moldura, 0.25f);
        }

        DrawRectangleRounded(b, 0.18f, 8, fundo);
        DrawRectangleRoundedLines(b, 0.18f, 8, borda);
        DrawCircleV((Vector2){b.x + 36.0f, b.y + b.height / 2.0f}, 22.0f, (Color){10, 8, 10, 255});
        DrawRing((Vector2){b.x + 36.0f, b.y + b.height / 2.0f}, 20.0f, 23.0f, 0.0f, 360.0f, 32, borda);
        const char *ls = TextFormat("%c", letra);
        DrawText(ls, (int)(b.x + 36.0f - MeasureText(ls, 26) / 2.0f), (int)(b.y + b.height / 2.0f - 13.0f), 26, est->texto);

        const char *txt = cartasTextoAlternativa(e->carta.alternativa[j]);
        float larguraTxt = b.width - 90.0f;
        int altTxt = cartasTextoQuebrado(txt, 0.0f, 0.0f, larguraTxt, 25, WHITE, 0, 0) - 8;
        cartasTextoQuebrado(txt, b.x + 76.0f, b.y + (b.height - (float)altTxt) / 2.0f, larguraTxt, 25, WHITE, 0, 1);
    }

    // rodape
    if (!resultado)
    {
        const char *dica = "Clique em uma alternativa ou tecle A, B, C ou D";
        DrawText(dica, (int)(l.painel.x + margem), (int)l.yRodape + 6, 24, (Color){200, 195, 185, 255});
        const char *pod = TextFormat("Poder atual: %d", arena->heroi.poder);
        DrawText(pod, (int)(l.painel.x + l.painel.width - margem - MeasureText(pod, 24)), (int)l.yRodape + 6, 24, (Color){110, 200, 255, 255});
    }
    else
    {
        const char *res = e->acertou ? "RESPOSTA CORRETA!" : "RESPOSTA INCORRETA!";
        Color corRes = e->acertou ? (Color){120, 240, 150, 255} : (Color){255, 110, 100, 255};
        DrawText(res, (int)(l.painel.x + margem), (int)l.yRodape, 34, corRes);
        const char *det = TextFormat("Poder: %d  ->  %d", e->poderAntes, e->poderDepois);
        DrawText(det, (int)(l.painel.x + l.painel.width - margem - MeasureText(det, 30)), (int)l.yRodape + 4, 30, (Color){255, 225, 110, 255});
        const char *cont = e->acertou ? "Clique ou tecle ENTER para continuar"
                                      : TextFormat("A correta era a letra %c. Clique ou tecle ENTER para continuar", correta);
        DrawText(cont, (int)(l.painel.x + margem), (int)l.yRodape + 46, 22, (Color){200, 195, 185, 255});
    }
}

/**
 * @brief Desenha a tela de cartas conforme a fase atual.
 * @param arena    Arena.
 * @param largTela Largura da tela.
 * @param altTela  Altura da tela.
 */
static void cartasDesenhar(const Arena *arena, float largTela, float altTela)
{
    const EstadoCartas *e = &estadoCartas;
    if (e->fase == CARTAS_FASE_INATIVA || e->fase == CARTAS_FASE_ESPERA)
        return;

    float alphaFundo = 0.78f;
    if (e->fase == CARTAS_FASE_ESCOLHA)
        alphaFundo = 0.78f * cartasClamp01(e->tempoFase * 3.0f);
    DrawRectangle(0, 0, (int)largTela, (int)altTela, Fade(BLACK, alphaFundo));

    switch (e->fase)
    {
    case CARTAS_FASE_ESCOLHA:
        cartasDesenharEscolha(largTela, altTela);
        break;
    case CARTAS_FASE_REVELANDO:
        cartasDesenharRevelando(largTela, altTela);
        break;
    case CARTAS_FASE_PERGUNTA:
        cartasDesenharPainel(arena, largTela, altTela, 0);
        break;
    case CARTAS_FASE_RESULTADO:
        cartasDesenharPainel(arena, largTela, altTela, 1);
        break;
    default:
        break;
    }
}

/**
 * @brief Resolve o combate entre o herói e um inimigo.
 *
 * Vence se o poder do herói for >= ao do inimigo, e então ABSORVE o poder
 * dele. Se perder, o herói recua e o poder não muda. Derrotar o Sif libera a
 * forma final; derrotar o último inimigo termina a arena; nos outros casos
 * abre a escolha de cartas.
 *
 * @param arena      Arena.
 * @param idxInimigo Índice do inimigo em @c arena->inimigos.
 */
static void executarCombate(Arena *arena, int idxInimigo)
{
    Inimigo *ini = &arena->inimigos[idxInimigo];
    if (ini->derrotado)
        return;

    arena->heroi.timerAtaque = 0.30f;

    if (arena->heroi.poder >= ini->poder)
    {
        int ganho = ini->poder;
        ini->derrotado = 1;
        long long novoPoder = (long long)arena->heroi.poder + (long long)ganho;
        arena->heroi.poder = clampIntMax(novoPoder, 1, PODER_MAX_INIMIGO * 2);

        adicionarPopup(TextFormat("+%d", ganho),
                       (Vector2){ini->pos.x, ini->pos.y - 150.0f},
                       (Color){90, 200, 255, 255});

        if (strcmp(ini->nome, "Sif") == 0)
        {
            arena->heroi.derrotouSif = 1;
        }

        atualizarEvolucaoHeroi(arena, 0);

        int restantes = 0;
        for (int i = 0; i < arena->qtdInimigos; i++)
        {
            if (!arena->inimigos[i].derrotado)
                restantes++;
        }
        if (restantes == 0)
        {
            arena->vitoriaArena = 1;
        }
        else
        {
            // Venceu o combate: o jogador escolhe uma carta de dificuldade
            cartasAgendarEscolha();
        }
    }
    else
    {
        arena->heroi.timerRecuo = 0.45f;
        arena->heroi.pos.x -= arena->heroi.direcaoX * 42.0f;
        arena->heroi.destino = arena->heroi.pos;

        adicionarPopup("PODER INSUFICIENTE!",
                       (Vector2){arena->heroi.pos.x, arena->heroi.pos.y - 170.0f},
                       (Color){255, 75, 75, 255});
    }
}

/**
 * @brief Decide o que fazer com um clique na arena.
 *
 * Clique em item: anda até ele para coletar. Clique em inimigo: anda até o
 * lado dele para lutar. Clique no chão: apenas anda até o ponto.
 *
 * @param arena  Arena.
 * @param clique Posição do clique.
 */
static void processarCliqueCenario(Arena *arena, Vector2 clique)
{
    if (!arena->heroi.vivo)
        return;

    // 1. Verifica clique nos itens
    for (int i = 0; i < arena->qtdItens; i++)
    {
        Item *it = &arena->itens[i];
        if (it->coletado)
            continue;

        Rectangle hitboxItem = {it->pos.x - 45.0f, it->pos.y - 95.0f, 90.0f, 105.0f};
        if (CheckCollisionPointRec(clique, hitboxItem))
        {
            float lado = (arena->heroi.pos.x <= it->pos.x) ? -48.0f : 48.0f;
            arena->heroi.destino = (Vector2){it->pos.x + lado, it->pos.y};
            arena->heroi.direcaoX = (arena->heroi.destino.x >= arena->heroi.pos.x) ? 1 : -1;
            arena->heroi.movendo = 1;
            arena->heroi.itemAlvoIdx = i;
            arena->heroi.inimigoAlvoIdx = -1;
            return;
        }
    }

    // 2. Verifica clique nos inimigos
    for (int i = 0; i < arena->qtdInimigos; i++)
    {
        Inimigo *ini = &arena->inimigos[i];
        if (ini->derrotado)
            continue;

        float largHit = ini->ehChefe ? 140.0f : 110.0f;
        float altHit = ini->ehChefe ? 175.0f : 155.0f;
        // pos é a base (pés) do inimigo, então a área clicável sobe a partir dela
        Rectangle hitboxIni = {ini->pos.x - largHit / 2.0f, ini->pos.y - altHit + 10.0f, largHit, altHit};

        if (CheckCollisionPointRec(clique, hitboxIni))
        {
            // o herói para ao lado do inimigo (do lado em que já está), não em cima dele
            float distanciaCombate = ini->ehChefe ? 82.0f : 68.0f;
            float lado = (arena->heroi.pos.x <= ini->pos.x) ? -distanciaCombate : distanciaCombate;
            arena->heroi.destino = (Vector2){ini->pos.x + lado, ini->pos.y};
            arena->heroi.direcaoX = (arena->heroi.destino.x >= arena->heroi.pos.x) ? 1 : -1;
            arena->heroi.movendo = 1;
            arena->heroi.inimigoAlvoIdx = i;
            arena->heroi.itemAlvoIdx = -1;
            return;
        }
    }

    // 3. Clique no chao: apenas anda ate o ponto clicado
    arena->heroi.destino = clique;
    arena->heroi.direcaoX = (clique.x >= arena->heroi.pos.x) ? 1 : -1;
    arena->heroi.movendo = 1;
    arena->heroi.itemAlvoIdx = -1;
    arena->heroi.inimigoAlvoIdx = -1;
}

/**
 * @brief Move o herói até o destino e, ao chegar, coleta o item ou luta.
 * @param arena Arena.
 * @param dt    Tempo do quadro em segundos.
 */
static void atualizarHeroi(Arena *arena, float dt)
{
    Heroi *heroi = &arena->heroi;

    if (heroi->timerAtaque > 0.0f)
        heroi->timerAtaque -= dt;
    if (heroi->timerRecuo > 0.0f)
        heroi->timerRecuo -= dt;
    if (heroi->timerLevelUp > 0.0f)
        heroi->timerLevelUp -= dt;

    if (!heroi->movendo)
        return;

    float dx = heroi->destino.x - heroi->pos.x;
    float dy = heroi->destino.y - heroi->pos.y;
    float dist = sqrtf(dx * dx + dy * dy);

    if (fabsf(dx) > 1.0f)
    {
        heroi->direcaoX = (dx >= 0.0f) ? 1 : -1;
    }

    // Chegou (tolerância de 5 px): só agora coleta o item ou inicia o combate
    if (dist <= 5.0f)
    {
        heroi->pos = heroi->destino;
        heroi->movendo = 0;

        if (heroi->itemAlvoIdx >= 0)
        {
            int idx = heroi->itemAlvoIdx;
            heroi->itemAlvoIdx = -1;
            coletarItem(arena, idx);
        }
        else if (heroi->inimigoAlvoIdx >= 0)
        {
            int idx = heroi->inimigoAlvoIdx;
            heroi->inimigoAlvoIdx = -1;
            heroi->direcaoX = (arena->inimigos[idx].pos.x >= heroi->pos.x) ? 1 : -1;
            executarCombate(arena, idx);
        }
    }
    else
    {
        float passo = heroi->velocidade * dt;
        if (passo > dist)
            passo = dist;
        heroi->pos.x += (dx / dist) * passo;
        heroi->pos.y += (dy / dist) * passo;
    }
}

/**
 * @brief Desenha um item flutuando, com nome e efeito.
 * @param it          Item.
 * @param selecionado 1 se é o alvo atual do herói (fica destacado).
 */
static void desenharItem(const Item *it, int selecionado)
{
    if (it->coletado)
        return;

    float t = (float)GetTime() + it->id * 0.9f;
    float flutuar = sinf(t * 3.2f) * 4.0f;

    if (selecionado)
    {
        DrawEllipseLines((int)it->pos.x, (int)it->pos.y, 34, 12, (Color){255, 225, 90, 230});
    }

    DrawEllipse((int)it->pos.x, (int)it->pos.y, 26, 9, (Color){0, 0, 0, 185});

    float topoY = it->pos.y - 68.0f;

    if (it->temTexItem)
    {
        float escala = 85.0f / (float)it->texItem.height;
        float larg = it->texItem.width * escala;
        float alt = it->texItem.height * escala;
        float offsetY = (it->tipo == TIPO_ITEM_ARMA_MULT) ? flutuar : 0.0f;

        Rectangle src = {0.0f, 0.0f, (float)it->texItem.width, (float)it->texItem.height};
        Rectangle dst = {it->pos.x, it->pos.y + offsetY, larg, alt};
        DrawTexturePro(it->texItem, src, dst, (Vector2){larg / 2.0f, alt}, 0.0f, WHITE);
        topoY = it->pos.y - alt + offsetY;
    }
    else if (it->tipo == TIPO_ITEM_ARMA_MULT)
    {
        float cy = it->pos.y - 44.0f + flutuar;
        DrawCircleGradient((int)it->pos.x, (int)cy, 38, (Color){255, 210, 60, 130}, (Color){255, 180, 0, 0});
        DrawLineEx((Vector2){it->pos.x - 16, cy + 20}, (Vector2){it->pos.x + 18, cy - 24}, 5.0f, (Color){255, 235, 130, 255});
        DrawLineEx((Vector2){it->pos.x - 18, cy + 8}, (Vector2){it->pos.x - 4, cy + 20}, 4.0f, (Color){210, 150, 40, 255});
        topoY = cy - 32.0f;
    }
    else
    {
        float bx = it->pos.x - 24.0f;
        float by = it->pos.y - 38.0f;
        DrawRectangleRounded((Rectangle){bx, by, 48, 36}, 0.2f, 6, (Color){125, 70, 25, 255});
        DrawRectangleRoundedLines((Rectangle){bx, by, 48, 36}, 0.2f, 6, (Color){245, 195, 65, 255});
        DrawRectangle((int)bx, (int)(by + 13), 48, 4, (Color){245, 195, 65, 255});
        DrawRectangle((int)(it->pos.x - 5), (int)(by + 10), 10, 10, (Color){255, 230, 110, 255});
        topoY = by - 6.0f;
    }

    const char *txt = (it->tipo == TIPO_ITEM_ARMA_MULT)
                          ? TextFormat("x%d", it->multiplicador)
                          : TextFormat("+%d", it->bonusPoder);

    Color corTexto = (it->tipo == TIPO_ITEM_ARMA_MULT)
                         ? (Color){255, 225, 80, 255}
                         : (Color){115, 195, 255, 255};

    int tamFonte = 24;
    int largTxt = MeasureText(txt, tamFonte);
    int tx = (int)(it->pos.x - largTxt / 2.0f);
    int ty = (int)topoY - 24;

    DrawText(txt, tx + 2, ty + 2, tamFonte, BLACK);
    DrawText(txt, tx - 1, ty - 1, tamFonte, BLACK);
    DrawText(txt, tx, ty, tamFonte, corTexto);
}

/**
 * @brief Desenha o herói com o sprite do estágio atual, animações e poder.
 *
 * Se a imagem do estágio não existir, usa um sprite anterior disponível.
 *
 * @param heroi Herói.
 */
static void desenharHeroi(const Heroi *heroi)
{
    MovimentoCorpo mov = calcularMovimentoPersonagem(1, heroi->movendo, heroi->direcaoX, 0.0f);

    float avancoGolpeX = 0.0f;
    if (heroi->timerAtaque > 0.0f)
    {
        float fator = sinf((heroi->timerAtaque / 0.30f) * PI);
        avancoGolpeX = heroi->direcaoX * fator * 26.0f;
        mov.inclinacao += heroi->direcaoX * fator * 14.0f;
    }

    // Aura de Level Up nos pés ao evoluir
    if (heroi->timerLevelUp > 0.0f)
    {
        float prog = 1.0f - (heroi->timerLevelUp / 1.05f);
        if (prog < 0.0f)
            prog = 0.0f;
        unsigned char alpha = (unsigned char)((1.0f - prog) * 235.0f);
        Color corAura = (heroi->estagioEvolucao >= 3) ? (Color){255, 215, 70, alpha} : (Color){110, 210, 255, alpha};
        DrawEllipseLines((int)(heroi->pos.x + avancoGolpeX), (int)heroi->pos.y,
                         (int)(36 + prog * 42), (int)(13 + prog * 16), corAura);
        DrawCircleGradient((int)(heroi->pos.x + avancoGolpeX), (int)(heroi->pos.y - 80.0f),
                           75.0f, (Color){corAura.r, corAura.g, corAura.b, (unsigned char)(alpha / 3)},
                           (Color){corAura.r, corAura.g, corAura.b, 0});
    }
    else if (heroi->estagioEvolucao >= 3)
    {
        DrawEllipseLines((int)(heroi->pos.x + avancoGolpeX), (int)heroi->pos.y,
                         46, 15, (Color){255, 210, 80, 175});
    }
    else if (heroi->estagioEvolucao >= 1)
    {
        DrawEllipseLines((int)(heroi->pos.x + avancoGolpeX), (int)heroi->pos.y,
                         35, 12, (Color){100, 200, 255, 155});
    }

    Color tint = (heroi->timerRecuo > 0.0f) ? (Color){255, 120, 120, 255} : WHITE;

    int raioSombra = (heroi->estagioEvolucao >= 3) ? 44 : 30;
    DrawEllipse((int)(heroi->pos.x + avancoGolpeX), (int)heroi->pos.y,
                (int)(raioSombra * mov.sombraEscala), (int)(11 * mov.sombraEscala), (Color){0, 0, 0, 195});

    float alturaAlvo = 155.0f;
    if (heroi->estagioEvolucao == 1)
        alturaAlvo = 164.0f;
    else if (heroi->estagioEvolucao == 2)
        alturaAlvo = 172.0f;
    else if (heroi->estagioEvolucao >= 3)
        alturaAlvo = 195.0f;

    float alturaTopo = heroi->pos.y - alturaAlvo + mov.passoY;

    // Seleciona o sprite do estágio atual; se a imagem daquele estágio não existir,
    // cai para um sprite anterior disponível (o jogo funciona mesmo sem todos os arquivos)
    Texture2D texAtiva = heroi->texHeroi;
    int temTexAtiva = heroi->temTexHeroi;

    if (heroi->estagioEvolucao >= 3 && heroi->temTexHeroiFinal)
    {
        texAtiva = heroi->texHeroiFinal;
        temTexAtiva = 1;
    }
    else if (heroi->estagioEvolucao >= 2 && heroi->temTexHeroiEspada2)
    {
        texAtiva = heroi->texHeroiEspada2;
        temTexAtiva = 1;
    }
    else if (heroi->estagioEvolucao >= 1 && heroi->temTexHeroiEspada1)
    {
        texAtiva = heroi->texHeroiEspada1;
        temTexAtiva = 1;
    }
    else if (heroi->estagioEvolucao >= 1 && heroi->temTexHeroiEspada2)
    {
        texAtiva = heroi->texHeroiEspada2;
        temTexAtiva = 1;
    }

    if (temTexAtiva)
    {
        float escalaBase = alturaAlvo / (float)texAtiva.height;
        float larg = texAtiva.width * escalaBase * mov.escalaX;
        float alt = texAtiva.height * escalaBase * mov.escalaY;

        // largura negativa no recorte espelha a imagem quando o herói anda para a esquerda
        float srcLarg = (heroi->direcaoX < 0) ? -(float)texAtiva.width : (float)texAtiva.width;
        Rectangle src = {0.0f, 0.0f, srcLarg, (float)texAtiva.height};

        Rectangle dst = {heroi->pos.x + avancoGolpeX, heroi->pos.y + mov.passoY, larg, alt};
        Vector2 pivoPes = {larg / 2.0f, alt}; // pivô nos pés: inclinação e posição partem do chão

        DrawTexturePro(texAtiva, src, dst, pivoPes, mov.inclinacao, tint);

        alturaTopo = heroi->pos.y - alt + mov.passoY;
    }

    if (heroi->timerAtaque > 0.0f)
    {
        float fx = heroi->pos.x + avancoGolpeX + (heroi->direcaoX * 44.0f);
        float fy = heroi->pos.y - 78.0f;
        Color corCorte = (heroi->estagioEvolucao >= 2) ? (Color){255, 220, 95, 235} : (Color){120, 210, 255, 215};
        DrawCircleGradient((int)fx, (int)fy, (heroi->estagioEvolucao >= 2) ? 46 : 34, corCorte, (Color){80, 160, 255, 0});
    }

    // Número flutuante acima da cabeça
    const char *txtPoder = TextFormat("%d", heroi->poder);
    int tamFonte = (heroi->estagioEvolucao >= 2) ? 31 : 28;
    int largTxt = MeasureText(txtPoder, tamFonte);

    int txtX = (int)(heroi->pos.x + avancoGolpeX - largTxt / 2.0f);
    int txtY = (int)alturaTopo - 28;

    Color corNumero = (heroi->estagioEvolucao >= 3)
                          ? (Color){255, 225, 95, 255}
                          : ((heroi->estagioEvolucao >= 1) ? (Color){115, 225, 255, 255} : (Color){80, 180, 255, 255});
    DrawText(txtPoder, txtX + 2, txtY + 2, tamFonte, BLACK);
    DrawText(txtPoder, txtX - 1, txtY - 1, tamFonte, BLACK);
    DrawText(txtPoder, txtX, txtY, tamFonte, corNumero);
}

/**
 * @brief Desenha um inimigo com nome e poder.
 * @param ini         Inimigo.
 * @param selecionado 1 se é o alvo atual do herói (fica destacado).
 */
static void desenharInimigo(const Inimigo *ini, int selecionado)
{
    if (ini->derrotado)
        return;

    MovimentoCorpo mov = calcularMovimentoPersonagem(0, 0, -1, ini->id * 1.3f);

    if (selecionado)
    {
        DrawEllipseLines((int)ini->pos.x, (int)ini->pos.y, ini->ehChefe ? 46 : 36, 14, (Color){255, 70, 70, 230});
    }

    DrawEllipse((int)ini->pos.x, (int)ini->pos.y, ini->ehChefe ? 38 : 28, 10, (Color){0, 0, 0, 195});

    float alturaBase = ini->ehChefe ? 175.0f : 145.0f;
    float alturaTopo = ini->pos.y - alturaBase;

    if (ini->temTexInimigo)
    {
        float escalaBase = alturaBase / (float)ini->texInimigo.height;
        float larg = ini->texInimigo.width * escalaBase * mov.escalaX;
        float alt = ini->texInimigo.height * escalaBase * mov.escalaY;

        Rectangle src = {0.0f, 0.0f, (float)ini->texInimigo.width, (float)ini->texInimigo.height};
        Rectangle dst = {ini->pos.x, ini->pos.y, larg, alt};
        Vector2 pivoPes = {larg / 2.0f, alt};

        DrawTexturePro(ini->texInimigo, src, dst, pivoPes, mov.inclinacao, WHITE);

        alturaTopo = ini->pos.y - alt;
    }

    const char *txtPoder = TextFormat("%d", ini->poder);
    int tamFonte = 28;
    int largTxt = MeasureText(txtPoder, tamFonte);

    int txtX = (int)(ini->pos.x - largTxt / 2.0f);
    int txtY = (int)alturaTopo - 28;

    DrawText(txtPoder, txtX + 2, txtY + 2, tamFonte, BLACK);
    DrawText(txtPoder, txtX - 1, txtY - 1, tamFonte, BLACK);
    DrawText(txtPoder, txtX, txtY, tamFonte, (Color){255, 65, 65, 255});
}

/**
 * @brief Monta uma arena nova: sorteia poderes, itens e posições.
 *
 * Os valores são calculados em cadeia para que SEMPRE exista uma ordem
 * vencível: Hollow Faca -> Baú 1 -> Hollow Tocha -> Baú 2 -> Espada
 * (multiplica) -> Sif -> Gwyn. O teto do Gwyn é sorteado entre 35 e
 * 9.999.999. As cartas de pergunta (acerto multiplica, erro reduz) ficam
 * por conta do jogador e podem melhorar ou piorar esse caminho.
 *
 * @param arena    Arena a preencher (o herói volta para o poder inicial).
 * @param largTela Largura da tela.
 * @param altTela  Altura da tela.
 */
/**
 * @brief Gera inimigos (incluindo Sif extra) e itens espalhados por toda a arena.
 */
static void gerarInimigosAleatorios(Arena *arena, float largTela, float altTela)
{
    Vector2 ocupados[32];
    int qtdOcupados = 0;

    // 1. Posição inicial do Herói
    arena->heroi.pos = (Vector2){largTela * 0.72f, altTela * 0.36f};
    arena->heroi.destino = arena->heroi.pos;
    arena->heroi.movendo = 0;
    arena->heroi.direcaoX = -1;
    arena->heroi.velocidade = 260.0f;
    arena->heroi.inimigoAlvoIdx = -1;
    arena->heroi.itemAlvoIdx = -1;
    arena->heroi.poder = PODER_INICIAL_HEROI;
    arena->heroi.nivelArma = 0;
    arena->heroi.estagioEvolucao = 0;
    arena->heroi.derrotouSif = 0;
    arena->heroi.vivo = 1;
    ocupados[qtdOcupados++] = arena->heroi.pos;

    // 2. Configura os 4 inimigos fixos (0: Hollow Faca, 1: Hollow Tocha, 2: Sif, 3: Gwyn)
    arena->inimigos[0].poder = GetRandomValue(3, 5);
    arena->inimigos[0].derrotado = 0;
    arena->inimigos[0].pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 95.0f);

    arena->inimigos[1].poder = GetRandomValue(6, 12);
    arena->inimigos[1].derrotado = 0;
    arena->inimigos[1].pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 95.0f);

    // Sif principal (Chefe)
    arena->inimigos[2].poder = GetRandomValue(45, 90);
    arena->inimigos[2].derrotado = 0;
    arena->inimigos[2].ehChefe = 1;
    arena->inimigos[2].pos = (Vector2){largTela * 0.25f, altTela * 0.34f};
    ocupados[qtdOcupados++] = arena->inimigos[2].pos;

    // Gwyn (Chefe final na arena)
    arena->inimigos[3].poder = sortearPoderAteMaximo(500, 2500);
    arena->inimigos[3].derrotado = 0;
    arena->inimigos[3].ehChefe = 1;
    arena->inimigos[3].pos = (Vector2){largTela * 0.84f, altTela * 0.56f};
    ocupados[qtdOcupados++] = arena->inimigos[3].pos;

    // 3. Adiciona mais 8 inimigos espalhados pela arena (incluindo mais um Sif)
    int totalInimigos = 12;
    if (totalInimigos > MAX_INIMIGOS)
        totalInimigos = MAX_INIMIGOS;

    const Vector2 pontosExtras[8] = {
        {largTela * 0.16f, altTela * 0.72f}, // Ruínas inferiores esquerda
        {largTela * 0.48f, altTela * 0.44f}, // Centro da arena (fogueira principal)
        {largTela * 0.52f, altTela * 0.75f}, // Plataforma inferior central
        {largTela * 0.72f, altTela * 0.72f}, // Fogueira inferior direita
        {largTela * 0.34f, altTela * 0.58f}, // Escadaria esquerda
        {largTela * 0.62f, altTela * 0.52f}, // Entrada direita da arena
        {largTela * 0.42f, altTela * 0.68f}, // Escadaria central
        {largTela * 0.20f, altTela * 0.54f}  // Platô esquerdo (segundo Sif)
    };

    for (int i = 4; i < totalInimigos; i++)
    {
        if (i == 11)
        {
            // Coloca mais um Sif como guardião extra
            arena->inimigos[i] = arena->inimigos[2];
            arena->inimigos[i].id = i + 1;
            strcpy(arena->inimigos[i].nome, "Sif Guardiao");
            arena->inimigos[i].ehChefe = 1;
            arena->inimigos[i].nivelDificuldade = 4;
            arena->inimigos[i].derrotado = 0;
            arena->inimigos[i].poder = GetRandomValue(150, 350);
            arena->inimigos[i].pos = pontosExtras[i - 4];
        }
        else
        {
            int tipoBase = (i % 2 == 0) ? 0 : 1; // Alterna entre Hollow Faca (0) e Hollow Tocha (1)
            arena->inimigos[i] = arena->inimigos[tipoBase];
            arena->inimigos[i].id = i + 1;
            arena->inimigos[i].ehChefe = 0;
            arena->inimigos[i].derrotado = 0;

            if (i < 7)
            {
                arena->inimigos[i].nivelDificuldade = 1;
                arena->inimigos[i].poder = GetRandomValue(4, 15);
            }
            else if (i < 10)
            {
                arena->inimigos[i].nivelDificuldade = 2;
                arena->inimigos[i].poder = GetRandomValue(18, 60);
            }
            else
            {
                arena->inimigos[i].nivelDificuldade = 3;
                arena->inimigos[i].poder = GetRandomValue(70, 140);
            }

            Vector2 base = pontosExtras[i - 4];
            arena->inimigos[i].pos = (Vector2){
                base.x + (float)GetRandomValue(-25, 25),
                base.y + (float)GetRandomValue(-18, 18)};
        }

        if (qtdOcupados < 32)
            ocupados[qtdOcupados++] = arena->inimigos[i].pos;
    }
    arena->qtdInimigos = totalInimigos;

    // 4. Gera 10 itens espalhados (7 baús de soma + 3 espadas multiplicadoras)
    int totalItens = 10;
    if (totalItens > MAX_ITENS)
        totalItens = MAX_ITENS;

    for (int i = 0; i < totalItens; i++)
    {
        memset(&arena->itens[i], 0, sizeof(Item));
        arena->itens[i].id = i + 1;
        arena->itens[i].coletado = 0;
        arena->itens[i].pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 75.0f);

        if (i == 2 || i == 6 || i == 9)
        {
            arena->itens[i].tipo = TIPO_ITEM_ARMA_MULT;
            strcpy(arena->itens[i].nome, "Espada Reluzente");
            arena->itens[i].multiplicador = (i == 9) ? GetRandomValue(6, 10) : GetRandomValue(2, 5);
        }
        else
        {
            arena->itens[i].tipo = TIPO_ITEM_BAU;
            strcpy(arena->itens[i].nome, "Bau de Almas");
            arena->itens[i].bonusPoder = GetRandomValue(3, 25);
        }
    }
    arena->qtdItens = totalItens;

    // 5. Limiares de evolução visual do herói
    arena->limiarEspada1 = 15;
    arena->limiarEspada2 = 80;
    arena->limiarFormaFinal = 400;
    arena->vitoriaArena = 0;
}

/**
 * @brief Alterna entre tela cheia e janela, reposicionando tudo na nova escala.
 * @param arena    Arena (posições são reescaladas).
 * @param largTela Largura atual; recebe a nova.
 * @param altTela  Altura atual; recebe a nova.
 */
static void alternarTelaCheia(Arena *arena, float *largTela, float *altTela)
{
    float antigaL = *largTela;
    float antigaA = *altTela;
    int monitor = GetCurrentMonitor();
    int monL = GetMonitorWidth(monitor);
    int monA = GetMonitorHeight(monitor);
    int novaL, novaA;

    if (IsWindowFullscreen())
    {
        ToggleFullscreen();
        novaL = (int)(monL * 0.8f);
        novaA = (int)(monA * 0.8f);
        SetWindowSize(novaL, novaA);
        SetWindowPosition((monL - novaL) / 2, (monA - novaA) / 2);
    }
    else
    {
        ToggleFullscreen();
        novaL = monL;
        novaA = monA;
    }

    *largTela = (float)novaL;
    *altTela = (float)novaA;

    float fx = *largTela / antigaL;
    float fy = *altTela / antigaA;

    arena->heroi.pos.x *= fx;
    arena->heroi.pos.y *= fy;
    arena->heroi.destino.x *= fx;
    arena->heroi.destino.y *= fy;

    for (int i = 0; i < arena->qtdInimigos; i++)
    {
        arena->inimigos[i].pos.x *= fx;
        arena->inimigos[i].pos.y *= fy;
    }
    for (int i = 0; i < arena->qtdItens; i++)
    {
        arena->itens[i].pos.x *= fx;
        arena->itens[i].pos.y *= fy;
    }
}

/**
 * @brief Desenha os 10 Hollows chorando espalhados no chão da arena, em tamanho menor e com lágrimas no rosto.
 */
/**
 * @brief Desenha os 10 Hollows chorando espalhados no chão ao fundo da arena, pequenos e com lágrimas nítidas.
 */
/**
 * @brief Desenha os 10 Hollows chorando espalhados na encosta ao fundo à direita (ponto de referência), um pouco maiores.
 */
static void desenharHollowsChorandoFundo(Texture2D texHollowChorando, int temTex, float largTela, float altTela)
{
    static int inicializado = 0;
    static float posXRel[10];
    static float posYRel[10];
    static float escalaRel[10];
    static int flipX[10];

    // Sorteia posições aleatórias na encosta ao fundo à direita (usando o cursor como referência)
    if (!inicializado)
    {
        for (int i = 0; i < 10; i++)
        {
            posXRel[i] = (float)GetRandomValue(66, 94) / 100.0f;   // Região direita ao fundo
            posYRel[i] = (float)GetRandomValue(22, 36) / 100.0f;   // Platô/encosta acima do Sif
            escalaRel[i] = (float)GetRandomValue(10, 13) / 100.0f; // Um pouco maiores (10% a 13%)
            flipX[i] = GetRandomValue(0, 1) ? 1 : -1;              // Lado aleatório
        }
        inicializado = 1;
    }

    float tempo = (float)GetTime();

    for (int i = 0; i < 10; i++)
    {
        float x = largTela * posXRel[i];
        float y = altTela * posYRel[i];
        float soluco = sinf(tempo * 5.2f + i * 1.7f) * 1.4f;

        // Sombra proporcional nos pés
        DrawEllipse((int)x, (int)y, 13.0f, 4.5f, (Color){0, 0, 0, 150});

        float wDesenhado = 34.0f;
        float hDesenhado = 76.0f;

        if (temTex && texHollowChorando.id > 0)
        {
            wDesenhado = texHollowChorando.width * escalaRel[i];
            hDesenhado = texHollowChorando.height * escalaRel[i];

            Rectangle src = {
                0.0f,
                0.0f,
                (float)texHollowChorando.width * flipX[i],
                (float)texHollowChorando.height};
            Rectangle dst = {x - wDesenhado * 0.5f, y - hDesenhado + soluco, wDesenhado, hDesenhado};
            DrawTexturePro(texHollowChorando, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, (Color){205, 210, 225, 245});
        }
        else
        {
            DrawCircle((int)x, (int)(y - 18.0f + soluco), 11.0f, (Color){75, 70, 68, 235});
            DrawCircle((int)x, (int)(y - 34.0f + soluco * 1.2f), 8.0f, (Color){115, 108, 100, 240});
        }

        // Lágrimas saindo do rosto/mãos (acompanhando o lado para onde o Hollow olha)
        float rostoX = x + (flipX[i] * wDesenhado * 0.16f);
        float rostoY = y - (hDesenhado * 0.84f) + soluco;
        float quedaMax = hDesenhado * 0.42f;

        float gota1 = fmodf(tempo * 28.0f + i * 9.0f, quedaMax);
        float gota2 = fmodf(tempo * 28.0f + i * 9.0f + (quedaMax * 0.5f), quedaMax);

        // Gota 1
        DrawCircle((int)(rostoX - 2.2f), (int)(rostoY + gota1), 2.2f, (Color){90, 185, 255, 235});
        DrawCircle((int)(rostoX - 2.2f), (int)(rostoY + gota1), 1.0f, (Color){225, 245, 255, 255});

        // Gota 2
        DrawCircle((int)(rostoX + 2.2f), (int)(rostoY + gota2), 2.2f, (Color){90, 185, 255, 235});
        DrawCircle((int)(rostoX + 2.2f), (int)(rostoY + gota2), 1.0f, (Color){225, 245, 255, 255});
    }
}
/**
 * @brief Abre a janela do raylib e roda a arena até o jogador sair.
 *
 * Carrega imagens (o jogo funciona mesmo sem elas), gera a arena, executa o
 * laço principal (F11 alterna tela cheia; ESC sai da tela cheia ou fecha) e
 * libera tudo no fim.
 *
 * @param banco Banco de perguntas usado pelas cartas.
 */
void executarInterfaceGrafica(Banco *banco)
{
    srand((unsigned int)time(NULL));
    SetRandomSeed((unsigned int)time(NULL));

    // Limpa avisos que sobraram de uma partida anterior (o vetor é static)
    memset(popups, 0, sizeof(popups));

    InitWindow(0, 0, "FLAME CHAOS");
    if (!IsWindowFullscreen())
    {
        ToggleFullscreen();
    }
    SetTargetFPS(60);
    ShowCursor();
    EnableCursor();
    SetExitKey(KEY_NULL); // ESC nao fecha mais o jogo direto

    // Cartas: precisa da janela criada (gera as texturas das costas)
    cartasInicializar(banco);

    float largTela = (float)GetScreenWidth();
    float altTela = (float)GetScreenHeight();

    Arena arena = {0};

    // Para cada imagem, tenta alguns caminhos (pasta assets, pasta acima, pasta atual)
    // e usa o primeiro que existir; sem imagem, o jogo roda sem ela (fundo preto).
    // Cenário panorâmico
    const char *caminhosCenario[4] = {"assets/dark.png", "assets/dark.jpg", "../assets/dark.png", "dark.png"};
    for (int i = 0; i < 4; i++)
    {
        if (FileExists(caminhosCenario[i]))
        {
            arena.cenario.texCenario = LoadTexture(caminhosCenario[i]);
            if (arena.cenario.texCenario.id > 0)
            {
                arena.cenario.temCenario = 1;
                break;
            }
        }
    }

    // Estágio 0: Herói inicial (assets/hero.png)
    const char *caminhosHeroi[4] = {"assets/hero.png", "assets/hero.jpg", "../assets/hero.png", "hero.png"};
    for (int i = 0; i < 4; i++)
    {
        arena.heroi.texHeroi = carregarSpriteRecortado(caminhosHeroi[i], &arena.heroi.temTexHeroi);
        if (arena.heroi.temTexHeroi)
            break;
    }

    // Estágio 1: Herói com Espada 1 (assets/hero_espada1.png)
    const char *caminhosEspada1[3] = {"assets/hero_espada1.png", "../assets/hero_espada1.png", "hero_espada1.png"};
    for (int i = 0; i < 3; i++)
    {
        arena.heroi.texHeroiEspada1 = carregarSpriteRecortado(caminhosEspada1[i], &arena.heroi.temTexHeroiEspada1);
        if (arena.heroi.temTexHeroiEspada1)
            break;
    }

    // Estágio 2: Herói com Espada 2 (assets/hero_espada2.png)
    const char *caminhosEspada2[3] = {"assets/hero_espada2.png", "../assets/hero_espada2.png", "hero_espada2.png"};
    for (int i = 0; i < 3; i++)
    {
        arena.heroi.texHeroiEspada2 = carregarSpriteRecortado(caminhosEspada2[i], &arena.heroi.temTexHeroiEspada2);
        if (arena.heroi.temTexHeroiEspada2)
            break;
    }

    // Estágio 3: Forma Suprema (assets/hero_final.png)
    const char *caminhosFinal[3] = {"assets/hero_final.png", "../assets/hero_final.png", "hero_final.png"};
    for (int i = 0; i < 3; i++)
    {
        arena.heroi.texHeroiFinal = carregarSpriteRecortado(caminhosFinal[i], &arena.heroi.temTexHeroiFinal);
        if (arena.heroi.temTexHeroiFinal)
            break;
    }

    // Texturas fixas dos inimigos
    arena.inimigos[0].id = 1;
    strcpy(arena.inimigos[0].nome, "Hollow Faca");
    arena.inimigos[0].nivelDificuldade = 1;
    arena.inimigos[0].texInimigo = carregarSpriteRecortado("assets/Hollow faca.png", &arena.inimigos[0].temTexInimigo);

    arena.inimigos[1].id = 2;
    strcpy(arena.inimigos[1].nome, "Hollow Tocha");
    arena.inimigos[1].nivelDificuldade = 2;
    arena.inimigos[1].texInimigo = carregarSpriteRecortado("assets/Hollow Tocha.png", &arena.inimigos[1].temTexInimigo);

    arena.inimigos[2].id = 3;
    strcpy(arena.inimigos[2].nome, "Sif");
    arena.inimigos[2].nivelDificuldade = 4;
    arena.inimigos[2].ehChefe = 1;
    arena.inimigos[2].texInimigo = carregarSpriteRecortado("assets/sif.png", &arena.inimigos[2].temTexInimigo);

    arena.inimigos[3].id = 4;
    strcpy(arena.inimigos[3].nome, "Gwyn");
    arena.inimigos[3].nivelDificuldade = 5;
    arena.inimigos[3].ehChefe = 1;
    arena.inimigos[3].texInimigo = carregarSpriteRecortado("assets/Gwyn.png", &arena.inimigos[3].temTexInimigo);

    // Textura opcional dos 10 Hollows chorando ao fundo
    int temTexHollowChorando = 0;
    Texture2D texHollowChorando = {0};
    const char *caminhosHollowChorando[4] = {
        "assets/Hollow chorado.png",
        "assets/hollow_chorado.png",
        "../assets/Hollow chorado.png",
        "Hollow chorado.png"};

    for (int i = 0; i < 4; i++)
    {
        texHollowChorando = carregarSpriteRecortado(caminhosHollowChorando[i], &temTexHollowChorando);
        if (temTexHollowChorando)
            break;
    }

    // Gera inimigos aleatórios (poder até 9999999) com herói começando em 5
    gerarInimigosAleatorios(&arena, largTela, altTela);

    // Posiciona o Gwyn em um local fixo na arena (altar/trono à direita)
    if (arena.qtdInimigos > 3)
    {
        arena.inimigos[3].pos = (Vector2){largTela * 0.84f, altTela * 0.56f};
    }

    int sair = 0;

    while (!WindowShouldClose() && !sair)
    {
        float dt = GetFrameTime();

        // F11: alterna tela cheia / janela | ESC: sai da tela cheia (ou fecha o jogo se ja estiver em janela)
        if (IsKeyPressed(KEY_F11))
        {
            alternarTelaCheia(&arena, &largTela, &altTela);
            if (arena.qtdInimigos > 3)
            {
                arena.inimigos[3].pos = (Vector2){largTela * 0.84f, altTela * 0.56f};
            }
        }
        else if (IsKeyPressed(KEY_ESCAPE))
        {
            if (IsWindowFullscreen())
            {
                alternarTelaCheia(&arena, &largTela, &altTela);
                if (arena.qtdInimigos > 3)
                {
                    arena.inimigos[3].pos = (Vector2){largTela * 0.84f, altTela * 0.56f};
                }
            }
            else
                sair = 1;
        }

        // Com as cartas na tela, o clique é das cartas; o cenário fica bloqueado
        if (cartasAtivo())
        {
            cartasAtualizar(&arena, dt, largTela, altTela);
        }
        else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            processarCliqueCenario(&arena, GetMousePosition());
        }

        atualizarHeroi(&arena, dt);

        BeginDrawing();
        ClearBackground(BLACK);

        if (arena.cenario.temCenario)
        {
            Rectangle src = {0.0f, 0.0f, (float)arena.cenario.texCenario.width, (float)arena.cenario.texCenario.height};
            Rectangle dst = {0.0f, 0.0f, largTela, altTela};
            DrawTexturePro(arena.cenario.texCenario, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        }

        // 10 Hollows chorando ao fundo da arena
        desenharHollowsChorandoFundo(texHollowChorando, temTexHollowChorando, largTela, altTela);

        for (int i = 0; i < arena.qtdItens; i++)
        {
            desenharItem(&arena.itens[i], (arena.heroi.itemAlvoIdx == i));
        }

        for (int i = 0; i < arena.qtdInimigos; i++)
        {
            desenharInimigo(&arena.inimigos[i], (arena.heroi.inimigoAlvoIdx == i));
        }

        desenharHeroi(&arena.heroi);

        atualizarEDesenharPopups(dt);

        cartasDesenhar(&arena, largTela, altTela);

        if (arena.vitoriaArena)
        {
            const char *msg = "INIMIGOS DERROTADOS!";
            int largMsg = MeasureText(msg, 28);
            DrawRectangle((int)(largTela / 2 - largMsg / 2 - 20), 28, largMsg + 40, 48, (Color){10, 10, 15, 220});
            DrawText(msg, (int)(largTela / 2 - largMsg / 2), 38, 28, (Color){255, 215, 80, 255});
        }

        Vector2 mousePos = GetMousePosition();
        DrawCircleV(mousePos, 7, WHITE);
        DrawCircleLinesV(mousePos, 7, BLACK);
        DrawCircleV(mousePos, 2, RED);

        EndDrawing();
    }

    for (int i = 0; i < arena.qtdInimigos; i++)
    {
        if (arena.inimigos[i].temTexInimigo)
            UnloadTexture(arena.inimigos[i].texInimigo);
    }
    for (int i = 0; i < arena.qtdItens; i++)
    {
        if (arena.itens[i].temTexItem)
            UnloadTexture(arena.itens[i].texItem);
    }
    if (temTexHollowChorando)
        UnloadTexture(texHollowChorando);
    if (arena.heroi.temTexHeroi)
        UnloadTexture(arena.heroi.texHeroi);
    if (arena.heroi.temTexHeroiEspada1)
        UnloadTexture(arena.heroi.texHeroiEspada1);
    if (arena.heroi.temTexHeroiEspada2)
        UnloadTexture(arena.heroi.texHeroiEspada2);
    if (arena.heroi.temTexHeroiFinal)
        UnloadTexture(arena.heroi.texHeroiFinal);
    if (arena.cenario.temCenario)
        UnloadTexture(arena.cenario.texCenario);
    cartasFinalizar();
    CloseWindow();
}

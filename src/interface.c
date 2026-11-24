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

#define MAX_INIMIGOS 16
#define MAX_ITENS 12
#define MAX_POPUPS 12
#define PODER_MAX_INIMIGO 9999999
#define PODER_INICIAL_HEROI 5

typedef enum
{
    TIPO_ITEM_BAU = 0,
    TIPO_ITEM_ARMA_MULT
} TipoItem;

typedef struct
{
    Vector2 pos;
    Vector2 destino;
    int movendo;
    int direcaoX; // 1 = direita, -1 = esquerda
    float velocidade;
    int inimigoAlvoIdx; // >= 0 se estiver indo atacar um inimigo
    int itemAlvoIdx;    // >= 0 se estiver indo pegar um item
    float timerAtaque;  // Animação de golpe ao chegar no inimigo
    float timerRecuo;   // Animação de dano caso perca o combate
    float timerLevelUp; // Efeito visual de evolução ao pegar item / upar
    int poder;
    int nivelArma;       // Quantidade de itens coletados
    int estagioEvolucao; // 0: hero, 1: hero_espada1, 2: hero_espada2, 3: hero_final
    int derrotouSif;     // 1 ao derrotar Sif (libera forma final)
    int vivo;
    Texture2D texHeroi;
    int temTexHeroi;
    Texture2D texHeroiEspada1;
    int temTexHeroiEspada1;
    Texture2D texHeroiEspada2;
    int temTexHeroiEspada2;
    Texture2D texHeroiFinal;
    int temTexHeroiFinal;
} Heroi;

typedef struct
{
    int id;
    char nome[48];
    Vector2 pos;
    int poder;
    int nivelDificuldade;
    int derrotado;
    int ehChefe;
    Texture2D texInimigo;
    int temTexInimigo;
} Inimigo;

typedef struct
{
    int id;
    char nome[48];
    TipoItem tipo;
    Vector2 pos;
    int bonusPoder;    // Ex: +4, +10...
    int multiplicador; // Ex: x4, x100...
    int coletado;
    Texture2D texItem;
    int temTexItem;
} Item;

typedef struct
{
    Texture2D texCenario;
    int temCenario;
} Cenario;

typedef struct
{
    int limiarEspada1;
    int limiarEspada2;
    int limiarFormaFinal;
    Cenario cenario;
    Heroi heroi;
    Inimigo inimigos[MAX_INIMIGOS];
    int qtdInimigos;
    Item itens[MAX_ITENS];
    int qtdItens;
    int vitoriaArena;
} Arena;

typedef struct
{
    float escalaX;
    float escalaY;
    float passoY;
    float inclinacao;
    float sombraEscala;
} MovimentoCorpo;

typedef struct
{
    int ativo;
    char texto[48];
    Vector2 pos;
    Color cor;
    float tempoRestante;
} TextoFlutuante;

static TextoFlutuante popups[MAX_POPUPS] = {0};

static int clampIntMax(long long valor, int minimo, int maximo)
{
    if (valor < minimo)
        return minimo;
    if (valor > maximo)
        return maximo;
    return (int)valor;
}

static int sortearPoderAteMaximo(int minimo, int maximo)
{
    if (maximo <= minimo)
        return minimo;
    unsigned int r1 = (unsigned int)(rand() & 0xFFFF);
    unsigned int r2 = (unsigned int)(rand() & 0xFFFF);
    unsigned int combinado = (r1 << 16) | r2;
    unsigned int intervalo = (unsigned int)(maximo - minimo + 1);
    return minimo + (int)(combinado % intervalo);
}

static Vector2 sortearPosicaoLivre(Vector2 ocupados[], int *qtdOcupados, float largTela, float altTela, float distMinima)
{
    Vector2 candidata = {largTela * 0.5f, altTela * 0.55f};

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

    if (*qtdOcupados < 32)
    {
        ocupados[*qtdOcupados] = candidata;
        (*qtdOcupados)++;
    }
    return candidata;
}

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

// Carrega o sprite removendo o fundo preto puro (ou branco se o canto for claro)
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

// Define o movimento corporal do personagem
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

// Avalia e atualiza o estágio visual do herói (0 -> 1 -> 2 -> 3)
// Forma Suprema (3) ativa ao derrotar o Sif (unindo-se a ele) ou atingir o poder necessário antes de enfrentar o Gwyn
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

// Aplica o efeito do item coletado e evolui o herói
static void coletarItem(Arena *arena, int idxItem)
{
    Item *it = &arena->itens[idxItem];
    if (it->coletado)
        return;

    it->coletado = 1;
    arena->heroi.timerLevelUp = 0.85f;
    arena->heroi.nivelArma++;

    if (it->tipo == TIPO_ITEM_ARMA_MULT)
    {
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
#define CARTAS_QTD 5
#define CARTA_TEX_L 360
#define CARTA_TEX_A 504

typedef enum
{
    CARTAS_FASE_INATIVA = 0,
    CARTAS_FASE_ESPERA,     // pequena pausa para ver o "+poder" do combate
    CARTAS_FASE_ESCOLHA,    // 5 cartas de costas na mesa
    CARTAS_FASE_REVELANDO,  // carta escolhida vai ao centro e desvira
    CARTAS_FASE_PERGUNTA,   // enunciado + alternativas A-D
    CARTAS_FASE_RESULTADO   // acerto/erro e consequencia no poder
} FaseCartas;

typedef struct
{
    FaseCartas fase;
    float tempoFase;
    int escolhida;
    int hover;
    int disponivel[CARTAS_QTD];
    float elevacao[CARTAS_QTD];
    CardGame carta;
    char letraEscolhida;
    int acertou;
    int poderAntes;
    int poderDepois;
} EstadoCartas;

typedef struct
{
    Color fundoTopo;
    Color fundoBase;
    Color moldura;
    Color molduraInterna;
    Color texto;
    Color contorno;
    Color brilho;
    float espacamento; // espaco entre letras
    float ondulacao;   // letras sobem e descem levemente
    int fonte;         // tamanho da palavra principal
    int pingos;        // 1 = letras "escorrendo" (podridao escarlate)
    int contornoDuplo; // 1 = segundo contorno dourado
} EstiloCarta;

typedef struct
{
    Rectangle painel;
    Rectangle botoes[4];
    float yCabecalho;
    float yEnunciado;
    float alturaEnunciado;
    float yRodape;
} LayoutPergunta;

static EstadoCartas estadoCartas;
static CardGame *todasCartas = NULL;
static CardGame *filtroCartas = NULL;
static int totalCartas = 0;
static RenderTexture2D costasCartas[CARTAS_QTD];
static int temCostasCartas[CARTAS_QTD];
static Texture2D artesCartas[CARTAS_QTD];
static int temArteCarta[CARTAS_QTD];
static Font fontesCartas[CARTAS_QTD];
static int temFonteCarta[CARTAS_QTD];

static const enum Dificuldade ordemDificuldades[CARTAS_QTD] = {MUITO_FACIL, FACIL, MEDIO, DIFICIL, MUITO_DIFICIL};
static const char *rotulosDificuldade[CARTAS_QTD] = {"MUITO FACIL", "FACIL", "MEDIO", "DIFICIL", "MUITO DIFICIL"};
static const char *nomesBossCarta[CARTAS_QTD] = {"MARGIT", "RENNALA", "RADAHN", "RADAGON", "MALENIA"};
static const char *arquivosBossCarta[CARTAS_QTD] = {"margit", "rennala", "radahn", "radagon", "malenia"};
static const char *numeraisCarta[CARTAS_QTD] = {"I", "II", "III", "IV", "V"};

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
static float cartasClamp01(float v)
{
    if (v < 0.0f)
        return 0.0f;
    if (v > 1.0f)
        return 1.0f;
    return v;
}

static float cartasSuavizar(float t)
{
    t = cartasClamp01(t);
    return 1.0f - powf(1.0f - t, 3.0f);
}

// Ruido deterministico (nao mexe no rand() do jogo)
static float cartasRuido(int semente)
{
    unsigned int x = (unsigned int)semente * 747796405u + 2891336453u;
    x = ((x >> ((x >> 28u) + 4u)) ^ x) * 277803737u;
    x = (x >> 22u) ^ x;
    return (float)(x & 0xFFFFu) / 65535.0f;
}

// Triangulo que se orienta sozinho (o raylib descarta triangulos no sentido errado)
static void cartasTri(Vector2 a, Vector2 b, Vector2 c, Color cor)
{
    float cruz = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (cruz > 0.0f)
        DrawTriangle(a, c, b, cor);
    else
        DrawTriangle(a, b, c, cor);
}

static void cartasQuad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color cor)
{
    cartasTri(a, b, c, cor);
    cartasTri(a, c, d, cor);
}

// Arco com espessura variavel (usado em chifres, aneis, cabelos...)
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

// Quebra o texto em linhas dentro de larguraMax. Retorna a altura usada.
// Com desenhar = 0 apenas mede. Com centralizar = 1, x e o centro das linhas.
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

// Se a alternativa ja vier como "A) texto", remove o prefixo (a letra e desenhada no botao)
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

static double cartasFatorAcerto(const CardGame *carta)
{
    double m = (double)carta->numMultiplicador;
    return (m > 0.0) ? m : 1.0;
}

// O redutor e aplicado como fator (ex.: 0.5 = metade). Se vier maior que 1, vira divisor.
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
static void cartasTextoTematico(int idx, const char *texto, float cx, float y, float fonte)
{
    const EstiloCarta *e = &estilosCartas[idx];
    Font f = temFonteCarta[idx] ? fontesCartas[idx] : GetFontDefault();
    int n = (int)strlen(texto);
    if (n > 31)
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

// "MUITO FACIL" vira duas linhas: MUITO (pequeno) + FACIL (grande)
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

static int cartasAtivo(void)
{
    return estadoCartas.fase != CARTAS_FASE_INATIVA;
}

static void cartasMudarFase(FaseCartas fase)
{
    estadoCartas.fase = fase;
    estadoCartas.tempoFase = 0.0f;
}

// Chamada quando o herói vence um combate
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
static Rectangle cartasRetanguloEscolha(int i, float largTela, float altTela)
{
    float cw = fminf(altTela * 0.50f * ((float)CARTA_TEX_L / (float)CARTA_TEX_A), largTela * 0.15f);
    float ch = cw * ((float)CARTA_TEX_A / (float)CARTA_TEX_L);
    float gap = cw * 0.22f;
    float total = CARTAS_QTD * cw + (CARTAS_QTD - 1) * gap;
    float x0 = (largTela - total) / 2.0f;
    return (Rectangle){x0 + i * (cw + gap), altTela * 0.54f - ch / 2.0f, cw, ch};
}

static LayoutPergunta cartasLayoutPergunta(float largTela, float altTela)
{
    LayoutPergunta l;
    const float margem = 44.0f, cabecalho = 84.0f, rodape = 96.0f, bh = 72.0f, gap = 14.0f;
    float pw = fminf(largTela * 0.82f, 1200.0f);

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
static void cartasSortearPergunta(int idx)
{
    EstadoCartas *e = &estadoCartas;
    int n = ProcurarDificuldadeCards(todasCartas, totalCartas, ordemDificuldades[idx], filtroCartas);
    if (n <= 0)
        return;

    EscolherCard(filtroCartas, &e->carta, n);
    e->escolhida = idx;
    e->hover = -1;
    cartasMudarFase(CARTAS_FASE_REVELANDO);
}

static void cartasResponder(Arena *arena, int indice)
{
    EstadoCartas *e = &estadoCartas;
    Heroi *heroi = &arena->heroi;

    e->letraEscolhida = (char)('A' + indice);
    e->acertou = (toupper((unsigned char)e->letraEscolhida) == toupper((unsigned char)e->carta.correta));
    e->poderAntes = heroi->poder;

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

static void cartasAtualizar(Arena *arena, float dt, float largTela, float altTela)
{
    EstadoCartas *e = &estadoCartas;
    e->tempoFase += dt;

    Vector2 mouse = GetMousePosition();
    int clicou = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

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

// Frente da carta revelada: mostra o que vale o acerto e o erro
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

// Resolve o combate entre o herói e o inimigo alvo
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

// Verifica clique em itens ou inimigos no cenário
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
        Rectangle hitboxIni = {ini->pos.x - largHit / 2.0f, ini->pos.y - altHit + 10.0f, largHit, altHit};

        if (CheckCollisionPointRec(clique, hitboxIni))
        {
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

// Atualiza a caminhada do herói e dispara coleta ou combate ao chegar no alvo
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

    // Seleciona o sprite correspondente ao estágio atual com fallback seguro
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

        float srcLarg = (heroi->direcaoX < 0) ? -(float)texAtiva.width : (float)texAtiva.width;
        Rectangle src = {0.0f, 0.0f, srcLarg, (float)texAtiva.height};

        Rectangle dst = {heroi->pos.x + avancoGolpeX, heroi->pos.y + mov.passoY, larg, alt};
        Vector2 pivoPes = {larg / 2.0f, alt};

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

// Gera inimigos aleatórios com poder até 9999999 (sem nível), enquanto o herói sempre começa em 5
static void gerarInimigosAleatorios(Arena *arena, float largTela, float altTela)
{
    arena->vitoriaArena = 0;

    int poderInicial = PODER_INICIAL_HEROI; // Herói sempre começa em 5

    // Sorteia o teto de poder do chefe final (Gwyn) entre 35 e 9999999
    int faixa = GetRandomValue(1, 4);
    int alvoGwyn = 60;
    if (faixa == 1)
        alvoGwyn = sortearPoderAteMaximo(35, 300);
    else if (faixa == 2)
        alvoGwyn = sortearPoderAteMaximo(301, 25000);
    else if (faixa == 3)
        alvoGwyn = sortearPoderAteMaximo(25001, 750000);
    else
        alvoGwyn = sortearPoderAteMaximo(750001, PODER_MAX_INIMIGO);

    // 1. Hollow Faca sempre vencível logo de início pelo herói (poder 1 a 5)
    int poderFaca = GetRandomValue(1, poderInicial);

    // 2. Baú 1 + Hollow Tocha (vencível após Hollow Faca + Baú 1)
    int bau1 = GetRandomValue(3, 8);
    long long poderAposFacaEBau1 = (long long)poderInicial + poderFaca + bau1;
    int poderTocha = clampIntMax(poderAposFacaEBau1 * GetRandomValue(65, 92) / 100, poderFaca + 1, PODER_MAX_INIMIGO - 10);

    // 3. Baú 2 e Espada Multiplicadora escalam a partir do 5 inicial para permitir alcançar Sif e Gwyn até 9999999
    long long poderAposTocha = poderAposFacaEBau1 + poderTocha;
    int bau2 = GetRandomValue(6, 20);
    long long baseAntesMult = poderAposTocha + bau2;

    int multArma = 4;
    if (alvoGwyn > 300)
    {
        long long multNecessario = (alvoGwyn / (baseAntesMult * 2)) + GetRandomValue(2, 4);
        multArma = clampIntMax(multNecessario, 4, 350000);
    }
    else
    {
        multArma = GetRandomValue(3, 5);
    }

    long long poderPreSif = baseAntesMult;
    long long poderComEspada = poderPreSif * (long long)multArma;

    // 4. Sif: fica entre a forma pré-espada e a forma com espada, garantindo vitória na ordem certa
    long long minSif = poderPreSif + 2;
    long long maxSif = (poderComEspada * 65) / 100;
    if (maxSif < minSif)
        maxSif = minSif;
    int poderSif = clampIntMax(minSif + (long long)((double)(maxSif - minSif) * (GetRandomValue(20, 80) / 100.0)),
                               poderTocha + 2, PODER_MAX_INIMIGO - 5);

    // 5. Gwyn: chefe final (até 9999999), vencível após coletar os itens e derrotar o Sif
    long long poderMaxAntesGwyn = poderComEspada + poderSif;
    int poderGwyn = clampIntMax(poderMaxAntesGwyn * GetRandomValue(68, 86) / 100, poderSif + 5, PODER_MAX_INIMIGO);

    // Limiares de evolução visual:
    // Forma Suprema (3) ativa ao derrotar o Sif (unindo-se a ele) ou atingir 75% do poder do Gwyn antes de enfrentá-lo
    arena->limiarEspada1 = clampIntMax((long long)poderInicial + bau1, poderInicial + 1, PODER_MAX_INIMIGO);
    arena->limiarEspada2 = clampIntMax(poderPreSif, arena->limiarEspada1 + 1, PODER_MAX_INIMIGO);
    arena->limiarFormaFinal = clampIntMax((long long)poderGwyn * 75 / 100, arena->limiarEspada2 + 1, poderGwyn);

    Vector2 ocupados[32];
    int qtdOcupados = 0;

    arena->heroi.pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 125.0f);
    arena->heroi.destino = arena->heroi.pos;
    arena->heroi.movendo = 0;
    arena->heroi.direcaoX = 1;
    arena->heroi.velocidade = 360.0f;
    arena->heroi.inimigoAlvoIdx = -1;
    arena->heroi.itemAlvoIdx = -1;
    arena->heroi.timerAtaque = 0.0f;
    arena->heroi.timerRecuo = 0.0f;
    arena->heroi.timerLevelUp = 0.0f;
    arena->heroi.poder = PODER_INICIAL_HEROI; // Sempre 5
    arena->heroi.nivelArma = 0;
    arena->heroi.estagioEvolucao = 0;
    arena->heroi.derrotouSif = 0;
    arena->heroi.vivo = 1;

    arena->qtdItens = 3;

    arena->itens[0].id = 1;
    strcpy(arena->itens[0].nome, "Bau de Almas Pequeno");
    arena->itens[0].tipo = TIPO_ITEM_BAU;
    arena->itens[0].pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 115.0f);
    arena->itens[0].bonusPoder = bau1;
    arena->itens[0].multiplicador = 1;
    arena->itens[0].coletado = 0;

    arena->itens[1].id = 2;
    strcpy(arena->itens[1].nome, "Bau de Almas Grande");
    arena->itens[1].tipo = TIPO_ITEM_BAU;
    arena->itens[1].pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 115.0f);
    arena->itens[1].bonusPoder = bau2;
    arena->itens[1].multiplicador = 1;
    arena->itens[1].coletado = 0;

    arena->itens[2].id = 3;
    snprintf(arena->itens[2].nome, sizeof(arena->itens[2].nome), "Espada do Abismo x%d", multArma);
    arena->itens[2].tipo = TIPO_ITEM_ARMA_MULT;
    arena->itens[2].pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 115.0f);
    arena->itens[2].bonusPoder = 0;
    arena->itens[2].multiplicador = multArma;
    arena->itens[2].coletado = 0;

    arena->qtdInimigos = 4;

    arena->inimigos[0].pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 125.0f);
    arena->inimigos[0].poder = poderFaca;
    arena->inimigos[0].derrotado = 0;

    arena->inimigos[1].pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 125.0f);
    arena->inimigos[1].poder = poderTocha;
    arena->inimigos[1].derrotado = 0;

    arena->inimigos[2].pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 135.0f);
    arena->inimigos[2].poder = poderSif;
    arena->inimigos[2].derrotado = 0;

    arena->inimigos[3].pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 145.0f);
    arena->inimigos[3].poder = poderGwyn;
    arena->inimigos[3].derrotado = 0;
}

// Alterna entre tela cheia e janela, reajustando as posicoes para o novo tamanho
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

void executarInterfaceGrafica(Banco *banco)
{
    srand((unsigned int)time(NULL));
    SetRandomSeed((unsigned int)time(NULL));

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

    // Gera inimigos aleatórios (poder até 9999999) com herói começando em 5
    gerarInimigosAleatorios(&arena, largTela, altTela);

    int sair = 0;

    while (!WindowShouldClose() && !sair)
    {
        float dt = GetFrameTime();

        // F11: alterna tela cheia / janela | ESC: sai da tela cheia (ou fecha o jogo se ja estiver em janela)
        if (IsKeyPressed(KEY_F11))
        {
            alternarTelaCheia(&arena, &largTela, &altTela);
        }
        else if (IsKeyPressed(KEY_ESCAPE))
        {
            if (IsWindowFullscreen())
                alternarTelaCheia(&arena, &largTela, &altTela);
            else
                sair = 1;
        }

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
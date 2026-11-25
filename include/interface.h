/**
 * @file interface.h
 * @brief Interface gráfica (raylib) do Flame Chaos: arena com herói,
 *        inimigos, itens e o sistema de cartas de perguntas.
 */

#ifndef INTERFACE_H
#define INTERFACE_H

#include "jogo_demilhao.h"

/**
 * @brief Executa a interface gráfica (raylib), controlada pelo mouse.
 *
 * Abre a janela em tela cheia, roda a arena até o jogador sair (ESC) e
 * fecha a janela, voltando ao menu do terminal.
 *
 * @param banco Banco de perguntas já carregado (usado pelas cartas).
 */
void executarInterfaceGrafica(Banco *banco);

#endif /* INTERFACE_H */

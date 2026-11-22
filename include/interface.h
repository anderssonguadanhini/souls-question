#ifndef INTERFACE_H
#define INTERFACE_H

#include "jogo_demilhao.h"

/**
 * @brief Executa a interface gráfica (Raylib) do Flame Chaos controlada 100% pelo mouse.
 * @param banco Ponteiro para o banco dinâmico de perguntas já carregado.
 */
void executarInterfaceGrafica(Banco *banco);

#endif // INTERFACE_H
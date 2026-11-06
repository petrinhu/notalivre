// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Canário do sanitizer (NL-3.0h; L-36). Defeito de propósito, um por modo. Fica fora do CTest: quem o roda é
// tools/portoes/sanitizer_ativo.py, no preset asan-ubsan, e exige o relatório. Sem sanitizer, os modos passam calados, e é isso
// que reprova no portão. A marca sai antes do defeito, para o CTest real decidir pelos regex do gate.
#include <climits>
#include <cstdio>
#include <cstring>
#include <thread>

namespace {

// Alocado e perdido numa thread que termina: é a forma medida (M5c). Objeto global e ponteiro local na pilha do main podem
// continuar alcançáveis na varredura do LeakSanitizer, sem relatório (M4, M4b e M6b).
void vazar()
{
    std::thread([] {
        int* volatile perdido = new int(1);
        static_cast<void>(perdido);
    }).join();
}

void estourarHeap()
{
    int* volatile bloco = new int[1];
    bloco[1] = 7;
    delete[] bloco;
}

void estourarInteiro()
{
    volatile int maximo = INT_MAX;
    volatile int um = 1;
    const int soma = maximo + um;
    std::printf("%d\n", soma);
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) {
        return 2;
    }
    std::puts("CANARIO-PASS");
    std::fflush(stdout);
    if (std::strcmp(argv[1], "ok") == 0) {
        return 0;
    }
    if (std::strcmp(argv[1], "leak") == 0) {
        vazar();
        return 0;
    }
    if (std::strcmp(argv[1], "heap") == 0) {
        estourarHeap();
        return 0;
    }
    if (std::strcmp(argv[1], "ub") == 0) {
        estourarInteiro();
        return 0;
    }
    return 2;
}

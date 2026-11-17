// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste do ClienteSefin: a sequência de requisições (METODO:status) lida do Log.
// Corpo movido byte a byte de apoio_cliente_sefin.h (NL-3.C, S5c, DA-3C.25). Namespace anônimo: uma cópia por executável.
#ifndef NL_TESTE_SEQUENCIA_REQUISICOES_H
#define NL_TESTE_SEQUENCIA_REQUISICOES_H

#include <sstream>
#include <string>
#include <vector>

namespace {

// Sequência de requisições do cliente, lida do Log: "METODO:status" por linha de evento sefin.requisicao.
std::vector<std::string> sequencia(const std::string& saida)
{
    std::vector<std::string> itens;
    std::istringstream linhas(saida);
    std::string linha;
    while (std::getline(linhas, linha)) {
        if (linha.find("\"evento\":\"sefin.requisicao\"") == std::string::npos) {
            continue;
        }
        const auto m = linha.find("\"metodo\":\"");
        const auto s = linha.find("\"status\":\"");
        if (m == std::string::npos || s == std::string::npos) {
            continue;
        }
        const auto metodo = linha.substr(m + 10, linha.find('"', m + 10) - (m + 10));
        const auto status = linha.substr(s + 10, linha.find('"', s + 10) - (s + 10));
        itens.push_back(metodo + ":" + status);
    }
    return itens;
}

} // namespace

#endif // NL_TESTE_SEQUENCIA_REQUISICOES_H

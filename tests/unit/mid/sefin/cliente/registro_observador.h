// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste do ClienteSefin: dublê da porta ObservadorRespostas.
// Corpo movido byte a byte de apoio_cliente_sefin.h (NL-3.C, S5c, DA-3C.25). Namespace anônimo: uma cópia por executável.
#ifndef NL_TESTE_REGISTRO_OBSERVADOR_H
#define NL_TESTE_REGISTRO_OBSERVADOR_H

#include <nl/mid/sefin/cliente/observador_respostas.h>
#include <QByteArray>
#include <string>
#include <vector>

namespace mc = nl::mid::sefin::cliente;

namespace {

// Observador de teste (NL-2.10c, passo 1): guarda as respostas HTTP que o cliente reporta, na ordem em que chegaram.
class RegistroObservador final : public mc::ObservadorRespostas {
public:
    struct Resposta {
        std::string metodo;
        std::string rota;
        int status = 0;
        QByteArray tipoConteudo;
        QByteArray corpo;
    };
    void registrar(std::string_view metodo, std::string_view rota, int status, const QByteArray& tipoConteudo,
                   const QByteArray& corpo) override
    {
        respostas.push_back(Resposta{std::string(metodo), std::string(rota), status, tipoConteudo, corpo});
    }
    std::vector<Resposta> respostas;
};

} // namespace

#endif // NL_TESTE_REGISTRO_OBSERVADOR_H

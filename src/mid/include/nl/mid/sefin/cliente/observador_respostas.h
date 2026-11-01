// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Observador das respostas HTTP da Sefin (NL-2.10c, DA-2.66). O ClienteSefin chama registrar depois de cada envio que devolveu
// resposta HTTP, inclusive recusas e tentativas repetidas (5xx). A rota é o modelo (/nfse, /dps/{id}, /nfse/{chave}), nunca o
// Id da DPS nem a chave. O corpo sai cru: quem grava é quem mascara (allowlist do CISO, N-03). Erro de gravação não pode
// desfazer a emissão, então o observador não devolve erro: quem implementa guarda o que falhou.
#ifndef NL_MID_SEFIN_CLIENTE_OBSERVADOR_RESPOSTAS_H
#define NL_MID_SEFIN_CLIENTE_OBSERVADOR_RESPOSTAS_H

#include <QByteArray>

#include <string_view>

namespace nl::mid::sefin::cliente {

class ObservadorRespostas {
public:
    virtual ~ObservadorRespostas() = default;

    /// Uma resposta HTTP da Sefin. tipoConteudo é o Content-Type recebido (vazio se o cabeçalho não veio).
    virtual void registrar(std::string_view metodo, std::string_view rota, int status, const QByteArray& tipoConteudo,
                           const QByteArray& corpo) = 0;
};

} // namespace nl::mid::sefin::cliente

#endif // NL_MID_SEFIN_CLIENTE_OBSERVADOR_RESPOSTAS_H

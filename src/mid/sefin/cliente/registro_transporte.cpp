// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Evento sefin.transporte_falhou (NL-2.10e, E4, DA-2.75). Ver registro_transporte.h. Cada campo sai de um conjunto fechado: o texto
// do Qt nunca entra, e o que não está no conjunto vira "outro".
#include <nl/mid/sefin/cliente/registro_transporte.h>

#include <string>
#include <vector>

namespace nl::mid::sefin::cliente {
namespace {

namespace fl = nl::foundation::log;
namespace ft = nl::foundation::transporte;

std::string causaParaLog(ft::CausaErro causa)
{
    switch (causa) {
    case ft::CausaErro::Timeout:
        return "timeout";
    case ft::CausaErro::Conexao:
        return "conexao";
    case ft::CausaErro::Tls:
        return "tls";
    case ft::CausaErro::Protocolo:
        return "protocolo";
    case ft::CausaErro::Configuracao:
        return "configuracao";
    }
    return "outro";
}

std::string etapaParaLog(std::string_view etapa)
{
    return (etapa == "POST" || etapa == "GET") ? std::string(etapa) : std::string("outro");
}

} // namespace

void registrarTransporteFalhou(fl::Log& registro, ft::CausaErro causa, int codigoQt, bool handshake, qint64 bytesEscritos,
                               std::string_view etapa)
{
    const std::vector<fl::Campo> campos{
        fl::Campo{"causa", causaParaLog(causa)},
        fl::Campo{"codigoQt", std::to_string(codigoQt)},
        fl::Campo{"handshake", std::string(handshake ? "sim" : "nao")},
        fl::Campo{"bytesEscritos", std::to_string(bytesEscritos < 0 ? 0 : bytesEscritos)},
        fl::Campo{"etapa", etapaParaLog(etapa)},
    };
    (void)registro.registrar(fl::Nivel::Erro, "sefin.transporte_falhou", campos);
}

} // namespace nl::mid::sefin::cliente

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

// Inteiros do QSslError::SslError, no máximo 16 (o que passar disso é cortado). Nenhum texto, nenhum certificado.
std::string codigosSslParaLog(const std::vector<int>& codigos)
{
    if (codigos.empty()) {
        return "nenhum";
    }
    std::string saida;
    const std::size_t limite = codigos.size() < 16 ? codigos.size() : 16;
    for (std::size_t i = 0; i < limite; ++i) {
        if (i > 0) {
            saida += ',';
        }
        saida += std::to_string(codigos[i]);
    }
    return saida;
}

std::string versaoTlsParaLog(std::string_view versao)
{
    return (versao == "tls1.2" || versao == "tls1.3" || versao == "nenhuma") ? std::string(versao) : std::string("outro");
}

std::string protocoloParaLog(std::string_view protocolo)
{
    return (protocolo == "http/1.1" || protocolo == "h2" || protocolo == "nenhum") ? std::string(protocolo) : std::string("outro");
}

} // namespace

void registrarTransporteFalhou(fl::Log& registro, ft::CausaErro causa, int codigoQt, bool handshake, qint64 bytesEscritos,
                               std::string_view etapa, const std::vector<int>& codigosSsl, std::string_view versaoTls,
                               std::string_view protocoloNegociado)
{
    const std::vector<fl::Campo> campos{
        fl::Campo{"causa", causaParaLog(causa)},
        fl::Campo{"codigoQt", std::to_string(codigoQt)},
        fl::Campo{"handshake", std::string(handshake ? "sim" : "nao")},
        fl::Campo{"bytesEscritos", std::to_string(bytesEscritos < 0 ? 0 : bytesEscritos)},
        fl::Campo{"etapa", etapaParaLog(etapa)},
        fl::Campo{"codigosSsl", codigosSslParaLog(codigosSsl)},
        fl::Campo{"versaoTls", versaoTlsParaLog(versaoTls)},
        fl::Campo{"protocoloNegociado", protocoloParaLog(protocoloNegociado)},
    };
    (void)registro.registrar(fl::Nivel::Erro, "sefin.transporte_falhou", campos);
}

} // namespace nl::mid::sefin::cliente

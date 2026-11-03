// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Allowlist do Log do cliente Sefin (NL-2.9, DA-2.54). Ver allowlist_log_sefin.h.
#include <nl/mid/sefin/cliente/allowlist_log_sefin.h>

namespace nl::mid::sefin::cliente {

namespace fl = nl::foundation::log;

std::vector<fl::DeclaracaoCampo> allowlistLogSefin()
{
    return {
        fl::DeclaracaoCampo{"metodo", fl::Regra::Cru},
        fl::DeclaracaoCampo{"status", fl::Regra::Cru},
        fl::DeclaracaoCampo{"codigoE", fl::Regra::Cru},
        fl::DeclaracaoCampo{"idDps", fl::Regra::InscricaoMascarada},
        fl::DeclaracaoCampo{"chaveAcesso", fl::Regra::InscricaoMascarada},
        // NL-2.10e (E4, DA-2.75): campos de conjunto fechado do evento sefin.transporte_falhou (valor fora do conjunto vira "outro").
        fl::DeclaracaoCampo{"causa", fl::Regra::Cru},
        fl::DeclaracaoCampo{"codigoQt", fl::Regra::Cru},
        fl::DeclaracaoCampo{"handshake", fl::Regra::Cru},
        fl::DeclaracaoCampo{"bytesEscritos", fl::Regra::Cru},
        fl::DeclaracaoCampo{"etapa", fl::Regra::Cru},
        // NL-2.10e (E4 complemento, DA-2.75): conjunto fechado de códigos SSL (inteiros), versão TLS e protocolo negociado.
        fl::DeclaracaoCampo{"codigosSsl", fl::Regra::Cru},
        fl::DeclaracaoCampo{"versaoTls", fl::Regra::Cru},
        fl::DeclaracaoCampo{"protocoloNegociado", fl::Regra::Cru},
        // NL-2.10e (DA-2.82): categoria do Qt num conjunto fechado (qt.network.ssl, qt.network.http2, qt.network.access, outro).
        fl::DeclaracaoCampo{"categoria", fl::Regra::Cru},
    };
}

std::vector<std::string> eventosLogSefin()
{
    return {"sefin.requisicao", "sefin.erro", "sefin.transporte_falhou", "emissao.aceita", "emissao.recusada"};
}

} // namespace nl::mid::sefin::cliente

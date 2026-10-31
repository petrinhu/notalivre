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
    };
}

std::vector<std::string> eventosLogSefin()
{
    return {"sefin.requisicao", "sefin.erro", "emissao.aceita", "emissao.recusada"};
}

} // namespace nl::mid::sefin::cliente

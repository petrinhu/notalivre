// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Âncoras de produção do A1 (NL-2.13, DA-2.44, DA-2.48, CISO 20:20:03 item 1). O conjunto é fixado no binário: o CMake
// embute o SHA-256 do SHA256SUMS versionado, e esta carga recusa qualquer manifesto com outro hash antes de ler uma raiz.
// Escolhe a finalidade "geral" e delega a conferência das raízes à Foundation (o motor pkcs12 não sabe o que é ICP).
// As raízes servem só de âncora da cadeia do A1 do próprio usuário (K2 do CLO, 30/10/2026).
#include <nl/mid/cofre/raizes_producao.h>

#include <nl/foundation/pkcs12/carga_raizes_icp.h>
#include <nl/foundation/pkcs12/erro_raiz_icp.h>

#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QFileDevice>
#include <QIODevice>

#include <cstddef>
#include <expected>
#include <string_view>

#ifndef NL_ICP_RAIZES_MANIFESTO_SHA256
#error "NL_ICP_RAIZES_MANIFESTO_SHA256 vem do CMake (third_party/icp-brasil): sem ele o conjunto de produção não é fixado"
#endif

namespace nl::mid::cofre {

namespace {

// Finalidade do conjunto de produção: só as raízes que o manifesto declara sem finalidade diferente (CISO, 30/10/2026).
constexpr std::string_view kProposito = "geral";

// SHA-256 do SHA256SUMS de produção, embutido pelo CMake. Nenhuma configuração do app troca este valor.
constexpr std::string_view kShaManifestoFixado = NL_ICP_RAIZES_MANIFESTO_SHA256;

nl::foundation::pkcs12::ErroCarga falhaDeProducao(nl::foundation::pkcs12::ErroRaizIcp motivo) {
    return nl::foundation::pkcs12::ErroCarga{motivo, {}};
}

} // namespace

// Confere o manifesto contra o hash embutido antes de tudo. A carga em si relê o SHA256SUMS: a janela entre as duas
// leituras existe, e o diretório de produção é de instalação (somente leitura para o usuário, por construção do pacote).
std::expected<nl::foundation::pkcs12::CargaRaizesIcp, nl::foundation::pkcs12::ErroCarga>
carregarRaizesDeProducao(const QString& diretorio, std::chrono::system_clock::time_point agora) {
    const QString manifesto = diretorio + QStringLiteral("/SHA256SUMS");
    QFile arquivo(manifesto);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return std::unexpected(falhaDeProducao(nl::foundation::pkcs12::ErroRaizIcp::ManifestoAusente));
    }
    const QByteArray conteudo = arquivo.readAll();
    if (arquivo.error() != QFileDevice::NoError) {
        return std::unexpected(falhaDeProducao(nl::foundation::pkcs12::ErroRaizIcp::FalhaInterna));
    }
    const QByteArray obtido = QCryptographicHash::hash(conteudo, QCryptographicHash::Sha256).toHex();
    if (std::string_view(obtido.constData(), static_cast<std::size_t>(obtido.size())) != kShaManifestoFixado) {
        return std::unexpected(falhaDeProducao(nl::foundation::pkcs12::ErroRaizIcp::ManifestoNaoFixado));
    }
    return nl::foundation::pkcs12::carregarRaizesIcp(diretorio, manifesto, kProposito, agora);
}

} // namespace nl::mid::cofre

// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste do ClienteSefin: a montagem do cliente (log, transporte e política de retry) contra o servidor falso.
// Corpo movido byte a byte de apoio_cliente_sefin.h (NL-3.C, S5c, DA-3C.25). Namespace anônimo: uma cópia por executável.
#ifndef NL_TESTE_MONTAGEM_CLIENTE_SEFIN_H
#define NL_TESTE_MONTAGEM_CLIENTE_SEFIN_H

#include <nl/foundation/log/log.h>
#include <nl/foundation/pkcs12/credencial_x509.h>
#include <nl/foundation/transporte/transporte_https.h>
#include <nl/mid/sefin/cliente/cliente_sefin.h>
#include <nl/mid/sefin/cliente/politica_retry.h>
#include <QSslCertificate>
#include <QString>
#include <chrono>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "fixture_certificado_teste.h"
#include "tls_cliente_teste.h"

using namespace std::chrono_literals;
namespace fl = nl::foundation::log;
namespace fp = nl::foundation::pkcs12;
namespace ft = nl::foundation::transporte;
namespace mc = nl::mid::sefin::cliente;

namespace {

struct Montagem {
    std::ostringstream destino;
    std::unique_ptr<fl::Log> log;
    std::unique_ptr<ft::TransporteHttps> transporte;
    std::unique_ptr<mc::ClienteSefin> cliente;
};

std::unique_ptr<Montagem> montar(int porta, const fp::CredencialX509& cred,
                                 std::shared_ptr<const mc::ComparadorInfDps> comparador = nullptr,
                                 mc::ObservadorRespostas* observador = nullptr)
{
    auto m = std::make_unique<Montagem>();
    m->log = std::make_unique<fl::Log>(
        m->destino, "sefin",
        std::vector<fl::DeclaracaoCampo>{{"metodo", fl::Regra::Cru}, {"status", fl::Regra::Cru},
                                         {"codigoE", fl::Regra::Cru}, {"etapa", fl::Regra::Cru}},
        std::vector<std::string>{"sefin.requisicao", "sefin.consulta", "sefin.emissao", "sefin.erro", "sefin.transporte_falhou"});
    const ft::UrlBase base{QUrl(QStringLiteral("https://127.0.0.1:%1").arg(porta)), QString(), 5000ms};
    m->transporte = std::make_unique<ft::TransporteHttps>(base, configuracaoTls(cred));
    mc::PoliticaRetry politica;
    politica.base = 1ms;  // a espera é real mas curta: o teste mede a ordem, não o tempo
    m->cliente = std::make_unique<mc::ClienteSefin>(*m->transporte, *m->log, politica, comparador, observador);
    return m;
}

// Cliente sem certificado de cliente: o servidor falso exige o certificado (CERT_REQUIRED), então a recusa é da camada TLS.
std::unique_ptr<Montagem> montarSemCertificado(int porta)
{
    auto m = std::make_unique<Montagem>();
    m->log = std::make_unique<fl::Log>(
        m->destino, "sefin",
        std::vector<fl::DeclaracaoCampo>{{"metodo", fl::Regra::Cru}, {"status", fl::Regra::Cru},
                                         {"codigoE", fl::Regra::Cru}, {"etapa", fl::Regra::Cru}},
        std::vector<std::string>{"sefin.requisicao", "sefin.consulta", "sefin.emissao", "sefin.erro", "sefin.transporte_falhou"});
    ft::ConfiguracaoTls tls;
    tls.autoridades = QSslCertificate::fromData(lerFixture(QStringLiteral("ac.pem")), QSsl::Pem);
    const ft::UrlBase base{QUrl(QStringLiteral("https://127.0.0.1:%1").arg(porta)), QString(), 5000ms};
    m->transporte = std::make_unique<ft::TransporteHttps>(base, tls);
    mc::PoliticaRetry politica;
    politica.base = 1ms;
    m->cliente = std::make_unique<mc::ClienteSefin>(*m->transporte, *m->log, politica);
    return m;
}

} // namespace

#endif // NL_TESTE_MONTAGEM_CLIENTE_SEFIN_H

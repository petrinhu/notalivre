// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Fachada do `notalivre emitir` (NL-2.9, DA-2.44). Ver emissao_cli.h. WIP: sem verificação.
#include <nl/mid/composicao/emissao_cli.h>

#include <nl/back/dps/dps_assinada.h>
#include <nl/back/dps/id_dps.h>
#include <nl/back/dps/numero_dps.h>
#include <nl/back/dps/serie.h>
#include <nl/back/identificacao/cnpj.h>
#include <nl/back/identificacao/codigo_municipio.h>
#include <nl/back/identificacao/cpf.h>
#include <nl/back/identificacao/inscricao.h>
#include <nl/back/app/emitir_nota.h>
#include <nl/foundation/log/log.h>
#include <nl/mid/composicao/confirmacao_inscricao.h>
#include <nl/mid/composicao/registro_cli.h>
#include <nl/foundation/pkcs12/ancoras.h>
#include <nl/foundation/pkcs12/carregador_pkcs12.h>
#include <nl/foundation/pkcs12/credencial_x509.h>
#include <nl/foundation/pkcs12/senha_segura.h>
#include <nl/foundation/transporte/transporte_https.h>
#include <nl/mid/composicao/emissao_nota.h>
#include <nl/mid/sefin/cliente/cliente_sefin.h>
#include <nl/mid/sefin/cliente/politica_retry.h>

#include <QFile>
#include <QIODevice>
#include <QSslCertificate>
#include <QSslKey>
#include <QUrl>
#include <QXmlStreamReader>

#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace nl::mid::composicao {
namespace {

namespace bk = nl::back;
namespace ft = nl::foundation::transporte;
namespace fp = nl::foundation::pkcs12;

FalhaEmissao falha(std::string codigo, std::string mensagem)
{
    return FalhaEmissao{std::move(codigo), std::move(mensagem)};
}

QByteArray lerArquivo(const QString& caminho)
{
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::ReadOnly)) {
        return {};
    }
    return arquivo.readAll();
}

// Id da DPS: "DPS" + municipio (7) + tipo (1) + inscricao (14) + serie (5) + numero (15). Os 45 caracteres são
// reconstruídos pelos tipos validados (IdDps::montar), então um Id mal formado do XML nunca chega ao envio.
std::expected<bk::IdDps, FalhaEmissao> idDoXml(const QByteArray& xml)
{
    QXmlStreamReader leitor(xml);
    QString id;
    while (!leitor.atEnd()) {
        if (leitor.readNextStartElement() && leitor.name() == QLatin1String("infDPS")) {
            id = leitor.attributes().value(QStringLiteral("Id")).toString();
            break;
        }
    }
    if (id.size() != 45 || !id.startsWith(QStringLiteral("DPS"))) {
        return std::unexpected(falha("emissao.id_dps_invalido", "o XML validado não traz o Id da DPS"));
    }
    auto municipio = bk::CodigoMunicipio::de(id.mid(3, 7));
    const QString inscricaoTexto = id.mid(11, 14);
    const bool cpfComZeros = inscricaoTexto.startsWith(QStringLiteral("000"));
    std::optional<bk::Inscricao> inscricao;
    if (cpfComZeros) {
        if (auto cpf = bk::Cpf::de(inscricaoTexto.mid(3))) {
            inscricao = bk::Inscricao{*cpf};
        }
    } else if (auto cnpj = bk::Cnpj::de(inscricaoTexto)) {
        inscricao = bk::Inscricao{*cnpj};
    }
    auto serie = bk::Serie::de(id.mid(25, 5).toInt());
    auto numero = bk::NumeroDps::de(id.mid(30, 15).toLongLong());
    if (!municipio || !inscricao || !serie || !numero) {
        return std::unexpected(falha("emissao.id_dps_invalido", "o Id da DPS não tem município, inscrição, série ou número válidos"));
    }
    auto idDps = bk::IdDps::montar(*municipio, *inscricao, *serie, *numero);
    if (!idDps) {
        return std::unexpected(falha("emissao.id_dps_invalido", "o Id da DPS não pode ser montado"));
    }
    return *idDps;
}

// TLS de cliente a partir da credencial: certificado DER; chave PKCS#8 em DER (NL-2.9, item 11). A chave sai do OpenSSL num
// buffer de memória do BIO, entregue ao QSslKey sem cópia (fromRawData), e o buffer é zerado logo depois.
ft::ConfiguracaoTls configuracaoTls(const fp::CredencialX509& credencial, const QByteArray& ancorasPem)
{
    ft::ConfiguracaoTls tls;
    unsigned char* der = nullptr;
    const int tamanho = i2d_X509(credencial.certificado(), &der);
    if (tamanho > 0 && der != nullptr) {
        tls.certificadoCliente = QSslCertificate(QByteArray(reinterpret_cast<const char*>(der), tamanho), QSsl::Der);
    }
    OPENSSL_free(der);
    BIO* saida = BIO_new(BIO_s_mem());
    if (saida != nullptr && i2d_PKCS8PrivateKey_bio(saida, credencial.chavePrivada(), nullptr, nullptr, 0, nullptr, nullptr) == 1) {
        char* dados = nullptr;
        const long n = BIO_get_mem_data(saida, &dados);
        if (n > 0 && dados != nullptr) {
            tls.chaveCliente = QSslKey(QByteArray::fromRawData(dados, static_cast<qsizetype>(n)), QSsl::Rsa, QSsl::Der);
            OPENSSL_cleanse(dados, static_cast<std::size_t>(n));
        }
    }
    BIO_free(saida);
    tls.autoridades = QSslCertificate::fromData(ancorasPem, QSsl::Pem);
    return tls;
}

FalhaEmissao falhaDoEnvio(const nl::back::FalhaSefin& falhaSefin)
{
    std::string codigo = falhaSefin.codigoE.empty() ? std::string("sefin.falha") : std::string("sefin.") + falhaSefin.codigoE;
    return falha(std::move(codigo), falhaSefin.mensagem);
}

} // namespace

namespace {

// Recusa da emissão: o registro recebe o evento com o Id e o código estável; o Id só entra depois que existe.
FalhaEmissao recusada(RegistroCli& registro, const bk::IdDps& idDps, FalhaEmissao falhaEmissao)
{
    registro.registrarEmissaoRecusada(idDps.texto().toStdString(), falhaEmissao.codigo);
    return falhaEmissao;
}

} // namespace

std::expected<QString, FalhaEmissao> emitirDpsPelaCli(const PedidoEmissao& pedido, fp::SenhaSegura& senha, RegistroCli& registro)
{
    // 1. DPS de entrada: JSON validado no XSD. Reprovação vem com o primeiro código do validador.
    auto xml = DpsValidar::executar(pedido.dpsJson, pedido.esquema);
    if (!xml) {
        const std::string codigo = xml.error().erros.empty() ? std::string("dps.invalida") : xml.error().erros.front().codigo();
        return std::unexpected(falha(codigo, "a DPS de entrada foi reprovada na validação"));
    }
    // 2. Id da DPS, a partir do próprio XML validado.
    auto idDps = idDoXml(*xml);
    if (!idDps) {
        return std::unexpected(idDps.error());
    }
    // 3. Âncoras e credencial do A1 (a senha é consumida aqui: o carregador a zera).
    const QByteArray ancorasPem = lerArquivo(pedido.caminhoAncoras);
    auto ancoras = fp::Ancoras::dePem(ancorasPem);
    if (!ancoras) {
        return std::unexpected(falha("credencial.ancoras_invalidas", "o arquivo de âncoras não pode ser lido"));
    }
    const QByteArray pfx = lerArquivo(pedido.caminhoCertificado);
    fp::CarregadorPkcs12 carregador;
    auto credencial = carregador.carregar(pfx, senha, *ancoras);
    if (!credencial) {
        return std::unexpected(recusada(registro, *idDps, falha("credencial.nao_carregou", "o certificado não carregou com a senha informada")));
    }
    // 4. Confirmação digitada: tem de bater com o A1 e com o emitente da DPS, antes de assinar e de enviar (NL-2.9, item 2).
    if (!pedido.confirmacao.isEmpty()) {
        const QString chaveDoEmitente = idDps->texto().mid(11, 14);
        if (auto conferido = conferirConfirmacao(pedido.confirmacao, credencial->inscricao(), chaveDoEmitente); !conferido) {
            return std::unexpected(recusada(registro, *idDps, falha(conferido.error().codigo(), conferido.error().mensagem())));
        }
    }
    // 5. Assinatura pela raiz de composição: a CLI não conhece o adaptador.
    EmissaoNota raiz(*credencial);
    auto assinado = raiz.emitirNota().executar(*xml, idDps->texto());
    if (!assinado) {
        return std::unexpected(recusada(registro, *idDps, falha(assinado.error().codigo(), assinado.error().mensagem())));
    }
    // 6. O XML assinado é o que vai à Sefin: revalidado no XSD antes do envio (NL-2.9, DA-2.51). Reprovação é código 4 na CLI.
    if (auto revalidado = DpsValidar::validarXml(*assinado, pedido.esquema); !revalidado) {
        const std::string codigo = revalidado.error().erros.empty() ? std::string("xsd.pos_assinatura") : revalidado.error().erros.front().codigo();
        return std::unexpected(recusada(registro, *idDps, falha(codigo, "o XML assinado não confere no XSD: nada foi enviado")));
    }
    // 7. Envio mTLS pelo cliente Sefin, com o Log do cliente (allowlist da Mid sefin, DA-2.54).
    const ft::UrlBase base{QUrl(pedido.urlSefin), QString(), std::chrono::milliseconds{30000}};
    ft::TransporteHttps transporte(base, configuracaoTls(*credencial, ancorasPem));
    nl::mid::sefin::cliente::ClienteSefin cliente(transporte, *registro.logDoCliente(), nl::mid::sefin::cliente::PoliticaRetry{});
    auto nota = cliente.emitir(bk::DpsAssinada{*idDps, *assinado});
    if (!nota) {
        return std::unexpected(recusada(registro, *idDps, falhaDoEnvio(nota.error())));
    }
    registro.registrarEmissaoAceita(idDps->texto().toStdString(), nota->chave.texto().toStdString());
    return nota->chave.texto();
}

} // namespace nl::mid::composicao

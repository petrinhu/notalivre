// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do adaptador do catálogo de localidades (NL-3.M, DA-3M.4). As tabelas de exemplo são gravadas numa pasta
// temporária pelo próprio teste (sem JSON versionado, que o portão SPDX exige declarar); cada teste que altera um
// arquivo altera só a cópia dele.
#include <nl/mid/tabelas/catalogo_localidades_embutido.h>

#include <nl/back/dps/codigo_pais.h>
#include <nl/back/identificacao/codigo_municipio.h>

#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTemporaryDir>
#include <QtTest>

#include <array>
#include <expected>
#include <string_view>

using nl::back::CodigoMunicipio;
using nl::back::CodigoPais;
using nl::mid::tabelas::CatalogoLocalidadesEmbutido;

namespace {

// Os três JSON com a forma de tools/tabelas: registros com "codigo" (municípios e localidades) e "iso2" (países).
const QByteArray kMunicipiosJson = QByteArray(R"JSON({"proveniencia":{"tabela":"municipios"},"registros":[)JSON"
                                              R"JSON({"codigo":"4305108","nome":"Caxias do Sul","uf":"RS"},)JSON"
                                              R"JSON({"codigo":"2611606","nome":"Recife","uf":"PE"}]})JSON");
const QByteArray kPaisesJson = QByteArray(R"JSON({"proveniencia":{"tabela":"paises"},"registros":[)JSON"
                                         R"JSON({"iso2":"US","nome":"Estados Unidos"},)JSON"
                                         R"JSON({"iso2":"AD","nome":"Andorra"}]})JSON");
const QByteArray kLocalidadesJson = QByteArray(R"JSON({"proveniencia":{"tabela":"localidades"},"registros":[)JSON"
                                              R"JSON({"codigo":"0000000","nome":"AGUAS MARITIMAS"}]})JSON");

// Nomes dos arquivos, na ordem em que o manifesto os lista (contrato de tools/tabelas).
const std::array<const char*, 3> kArquivos{"municipios.json", "paises.json", "localidades.json"};
const std::array<QByteArray, 3> kConteudos{kMunicipiosJson, kPaisesJson, kLocalidadesJson};

QByteArray sha256Hex(const QByteArray& bytes)
{
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
}

std::string_view vista(const QByteArray& bytes)
{
    return std::string_view(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

bool gravaArquivo(const QString& caminho, const QByteArray& bytes)
{
    QFile arquivo(caminho);
    if (!arquivo.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    return arquivo.write(bytes) == bytes.size();
}

// Regrava o SHA256SUMS a partir dos JSON que estão na pasta (formato sha256sum: hash, dois espaços, nome). Devolve o
// pino, que é o SHA-256 do próprio SHA256SUMS. Vazio se a escrita falhar.
QByteArray regravaManifesto(const QString& dir)
{
    QByteArray manifesto;
    for (const char* nome : kArquivos) {
        QFile arquivo(QDir(dir).filePath(QString::fromLatin1(nome)));
        if (!arquivo.open(QIODevice::ReadOnly)) {
            return {};
        }
        manifesto += sha256Hex(arquivo.readAll()) + "  " + nome + "\n";
    }
    if (!gravaArquivo(QDir(dir).filePath(QStringLiteral("SHA256SUMS")), manifesto)) {
        return {};
    }
    return sha256Hex(manifesto);
}

// Grava as três tabelas de exemplo e o manifesto delas em `dir`. Devolve o pino. Vazio se a escrita falhar.
QByteArray gravaTabelas(const QString& dir)
{
    if (!QDir().mkpath(dir)) {
        return {};
    }
    for (std::size_t i = 0; i < kArquivos.size(); ++i) {
        if (!gravaArquivo(QDir(dir).filePath(QString::fromLatin1(kArquivos[i])), kConteudos[i])) {
            return {};
        }
    }
    return regravaManifesto(dir);
}

} // namespace

class TstCatalogoLocalidades : public QObject
{
    Q_OBJECT

private slots:
    // Com as tabelas e o pino certos, o catálogo carrega e responde: município de municipios.json e de localidades.json
    // existe, país ISO2 existe, código ausente não existe. Este slot é o caminho feliz; o do build entra no passo 1.
    void fixtureCarregaERespondeExistencia();

    // SHA256SUMS com hash diferente do pino recebido é recusado antes de qualquer JSON ser lido (configuração, código 10).
    void manifestoForaDoPinRecusado();

    // JSON cujo SHA-256 não confere com o manifesto é recusado, mesmo com o pino certo.
    void tabelaDivergenteDoManifestoRecusada();

    // Pasta inexistente, ou um dos JSON faltando no diretório, é recusado como ausente.
    void tabelaAusenteRecusada();

    // JSON malformado, ou registro com código fora do domínio, é recusado depois da conferência do SHA-256.
    void jsonMalformadoRecusado();

    // SHA256SUMS fora do formato sha256sum, ou com tabela repetida, é recusado como manifesto inválido (pino batendo).
    void manifestoMalformadoRecusado();
};

void TstCatalogoLocalidades::fixtureCarregaERespondeExistencia()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString dir = temp.path() + QStringLiteral("/tabelas");
    const QByteArray pino = gravaTabelas(dir);
    QVERIFY(!pino.isEmpty());

    const auto resultado = CatalogoLocalidadesEmbutido::carregar(dir, vista(pino));
    QVERIFY2(resultado.has_value(), "tabelas íntegras com o pino certo têm de carregar");
    const CatalogoLocalidadesEmbutido& catalogo = resultado.value();

    QVERIFY(catalogo.municipioExiste(CodigoMunicipio::de(QStringLiteral("4305108")).value()));
    QVERIFY(catalogo.municipioExiste(CodigoMunicipio::de(QStringLiteral("2611606")).value()));
    QVERIFY2(catalogo.municipioExiste(CodigoMunicipio::de(QStringLiteral("0000000")).value()),
             "a localidade 0000000 (Águas Marítimas) é consultada como município (DA-1.10)");
    QVERIFY(!catalogo.municipioExiste(CodigoMunicipio::de(QStringLiteral("9999999")).value()));
    QVERIFY(catalogo.paisExiste(CodigoPais::de(QStringLiteral("US")).value()));
    QVERIFY(catalogo.paisExiste(CodigoPais::de(QStringLiteral("AD")).value()));
    QVERIFY(!catalogo.paisExiste(CodigoPais::de(QStringLiteral("XX")).value()));
}

void TstCatalogoLocalidades::manifestoForaDoPinRecusado()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString dir = temp.path() + QStringLiteral("/tabelas");
    QVERIFY(!gravaTabelas(dir).isEmpty());

    const QByteArray pinoZerado(64, '0');
    const auto resultado = CatalogoLocalidadesEmbutido::carregar(dir, vista(pinoZerado));
    QVERIFY2(!resultado.has_value(), "SHA256SUMS fora do pino tem de ser recusado");
    QCOMPARE(QString::fromStdString(resultado.error().codigo()), QStringLiteral("configuracao.tabelas_manifesto_nao_fixado"));
    QVERIFY2(!QString::fromStdString(resultado.error().mensagem()).contains(dir), "a mensagem não pode trazer o caminho");
}

void TstCatalogoLocalidades::tabelaDivergenteDoManifestoRecusada()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString dir = temp.path() + QStringLiteral("/tabelas");
    const QByteArray pino = gravaTabelas(dir);
    QVERIFY(!pino.isEmpty());

    // Altera um byte só do municipios.json; o SHA256SUMS fica igual, então o pino continua valendo.
    QFile municipios(QDir(dir).filePath(QStringLiteral("municipios.json")));
    QVERIFY(municipios.open(QIODevice::Append));
    QVERIFY(municipios.write(" ") == 1);
    municipios.close();

    const auto resultado = CatalogoLocalidadesEmbutido::carregar(dir, vista(pino));
    QVERIFY2(!resultado.has_value(), "JSON divergente do manifesto tem de ser recusado");
    QCOMPARE(QString::fromStdString(resultado.error().codigo()), QStringLiteral("tabelas.arquivo_divergente"));
    QVERIFY2(!QString::fromStdString(resultado.error().mensagem()).contains(dir), "a mensagem não pode trazer o caminho");
}

void TstCatalogoLocalidades::tabelaAusenteRecusada()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    // Pasta que não existe: nem o SHA256SUMS abre.
    const QString inexistente = temp.path() + QStringLiteral("/nao_existe");
    const QByteArray pinoQualquer(64, 'a');
    const auto semPasta = CatalogoLocalidadesEmbutido::carregar(inexistente, vista(pinoQualquer));
    QVERIFY2(!semPasta.has_value(), "pasta inexistente tem de ser recusada");
    QCOMPARE(QString::fromStdString(semPasta.error().codigo()), QStringLiteral("tabelas.ausente"));

    // Pasta certa, mas sem paises.json: o SHA256SUMS confere, e a ausência é achada antes do hash.
    const QString dir = temp.path() + QStringLiteral("/tabelas");
    const QByteArray pino = gravaTabelas(dir);
    QVERIFY(!pino.isEmpty());
    QVERIFY(QFile::remove(QDir(dir).filePath(QStringLiteral("paises.json"))));

    const auto semJson = CatalogoLocalidadesEmbutido::carregar(dir, vista(pino));
    QVERIFY2(!semJson.has_value(), "JSON faltando tem de ser recusado");
    QCOMPARE(QString::fromStdString(semJson.error().codigo()), QStringLiteral("tabelas.ausente"));
    QVERIFY2(!QString::fromStdString(semJson.error().mensagem()).contains(dir), "a mensagem não pode trazer o caminho");
}

void TstCatalogoLocalidades::jsonMalformadoRecusado()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString dir = temp.path() + QStringLiteral("/tabelas");
    QVERIFY(!gravaTabelas(dir).isEmpty());

    // Caso 1: JSON que não faz parse. O manifesto é regravado com o hash novo, então o SHA-256 confere e o erro é do parse.
    QVERIFY(gravaArquivo(QDir(dir).filePath(QStringLiteral("municipios.json")), QByteArray("{ \"registros\": [")));
    const QByteArray pinoMalformado = regravaManifesto(dir);
    QVERIFY(!pinoMalformado.isEmpty());
    const auto malformado = CatalogoLocalidadesEmbutido::carregar(dir, vista(pinoMalformado));
    QVERIFY2(!malformado.has_value(), "JSON malformado tem de ser recusado");
    QCOMPARE(QString::fromStdString(malformado.error().codigo()), QStringLiteral("tabelas.json_invalido"));

    // Caso 2: JSON válido com um código de município fora do domínio (7 dígitos). A mensagem não ecoa o valor.
    QVERIFY(gravaArquivo(QDir(dir).filePath(QStringLiteral("municipios.json")),
                         QByteArray(R"JSON({"registros":[{"codigo":"123456789"}]})JSON")));
    const QByteArray pinoForaDoDominio = regravaManifesto(dir);
    QVERIFY(!pinoForaDoDominio.isEmpty());
    const auto foraDoDominio = CatalogoLocalidadesEmbutido::carregar(dir, vista(pinoForaDoDominio));
    QVERIFY2(!foraDoDominio.has_value(), "código fora do domínio tem de ser recusado");
    QCOMPARE(QString::fromStdString(foraDoDominio.error().codigo()), QStringLiteral("tabelas.json_invalido"));
    QVERIFY2(!QString::fromStdString(foraDoDominio.error().mensagem()).contains(QStringLiteral("123456789")),
             "a mensagem não pode ecoar o conteúdo do registro");
}

void TstCatalogoLocalidades::manifestoMalformadoRecusado()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString dir = temp.path() + QStringLiteral("/tabelas");
    QVERIFY(!gravaTabelas(dir).isEmpty());
    const QByteArray hashMunicipios = sha256Hex(kMunicipiosJson);
    const QByteArray hashPaises = sha256Hex(kPaisesJson);
    const QByteArray hashLocalidades = sha256Hex(kLocalidadesJson);

    // Caso 1: um espaço só entre o hash e o nome (o formato sha256sum pede dois).
    const QByteArray umEspaco = hashMunicipios + " municipios.json\n" + hashPaises + "  paises.json\n"
                                + hashLocalidades + "  localidades.json\n";
    // Caso 2: municipios.json repetido e localidades.json ausente do manifesto.
    const QByteArray repetido = hashMunicipios + "  municipios.json\n" + hashMunicipios + "  municipios.json\n"
                                + hashPaises + "  paises.json\n";

    const std::array<QByteArray, 2> casos{umEspaco, repetido};
    for (const QByteArray& manifesto : casos) {
        // O pino é o SHA-256 deste próprio SHA256SUMS, então a carga passa do pino e chega ao parse do manifesto.
        QVERIFY(gravaArquivo(QDir(dir).filePath(QStringLiteral("SHA256SUMS")), manifesto));
        const QByteArray pino = sha256Hex(manifesto);
        const auto resultado = CatalogoLocalidadesEmbutido::carregar(dir, vista(pino));
        QVERIFY2(!resultado.has_value(), "manifesto fora do formato ou com tabela repetida tem de ser recusado");
        QCOMPARE(QString::fromStdString(resultado.error().codigo()), QStringLiteral("tabelas.manifesto_invalido"));
        QVERIFY2(!QString::fromStdString(resultado.error().mensagem()).contains(dir), "a mensagem não pode trazer o caminho");
        QVERIFY2(!QString::fromStdString(resultado.error().mensagem()).contains(QString::fromLatin1(hashMunicipios)),
                 "a mensagem não pode ecoar o conteúdo do manifesto");
    }
}

QTEST_APPLESS_MAIN(TstCatalogoLocalidades)
#include "tst_catalogo_localidades.moc"

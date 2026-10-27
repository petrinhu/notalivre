// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do tomador e dos textos de pessoa (NL-1.4): identificação por CPF, CNPJ, NIF ou cNaoNIF; nome opcional pelo leiaute de 20/10.

#include <nl/back/dps/tomador.h>

#include <QtTest>

#include "ajudantes.h"

using nl::back::Cpf;
using nl::back::EmailPessoa;
using nl::back::IdentificacaoPessoa;
using nl::back::InscricaoMunicipal;
using nl::back::MotivoNaoNif;
using nl::back::NomeRazaoSocial;
using nl::back::Nif;
using nl::back::Tomador;

class TstPessoas : public QObject
{
    Q_OBJECT

private slots:
    void tomadorComCpfCnpjOuNif();
    void tomadorSemNomeAceitoPeloLeiaute();
    void tomadorEstrangeiroComNifOuCNaoNif();
    void nifComMaisDeQuarentaCaracteresRejeitado();
    void nomeNoLimiteDeTrezentosAceito();
    void nomeComMaisDeTrezentosRejeitado();
    void inscricaoMunicipalAteQuinzeCaracteres();
    void emailAteOitentaCaracteres();
    void nomeVazioDoTomadorDevolveErroComCodigo();
};

void TstPessoas::tomadorComCpfCnpjOuNif()
{
    const auto cpf = nl::back::Cpf::de(u"529.982.247-25");
    QVERIFY(cpf.has_value());
    const Tomador tomadorCpf = Tomador::montar(IdentificacaoPessoa{*cpf}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt);
    QVERIFY(std::holds_alternative<Cpf>(tomadorCpf.identificacao()));

    const auto cnpj = nl::back::Cnpj::de(u"11222333000181");
    QVERIFY(cnpj.has_value());
    const Tomador tomadorCnpj = Tomador::montar(IdentificacaoPessoa{*cnpj}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt);
    QVERIFY(std::holds_alternative<nl::back::Cnpj>(tomadorCnpj.identificacao()));

    const auto nif = Nif::de(u"ABC123456", "toma/NIF");
    QVERIFY(nif.has_value());
    const Tomador tomadorNif = Tomador::montar(IdentificacaoPessoa{*nif}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt);
    QVERIFY(std::holds_alternative<Nif>(tomadorNif.identificacao()));

    const auto motivo = MotivoNaoNif::de(u"2");
    QVERIFY(motivo.has_value());
    const Tomador tomadorSemNif = Tomador::montar(IdentificacaoPessoa{*motivo}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt);
    QVERIFY(std::holds_alternative<MotivoNaoNif>(tomadorSemNif.identificacao()));
}

void TstPessoas::tomadorSemNomeAceitoPeloLeiaute()
{
    // toma/xNome é 0-1 no Anexo de 20/10 (linha Excel 153); no de 09/02 era 1-1 (linha Excel 151). A tela que exige o nome é a linha 5 da INBOX.
    const auto cpf = nl::back::Cpf::de(u"52998224725");
    QVERIFY(cpf.has_value());
    const Tomador tomador = Tomador::montar(IdentificacaoPessoa{*cpf}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt);
    QVERIFY(!tomador.xNome().has_value());
}

void TstPessoas::tomadorEstrangeiroComNifOuCNaoNif()
{
    // DA-1.17: tomador no exterior se identifica por NIF ou por cNaoNIF (tiposComplexos_v1.01.xsd:1008-1020, escolha de TCInfoPessoa).
    const auto nif = Nif::de(u"ABC123456", "toma/NIF");
    const auto motivo = MotivoNaoNif::de(u"1");
    QVERIFY(nif.has_value());
    QVERIFY(motivo.has_value());
    const Tomador comNif = Tomador::montar(IdentificacaoPessoa{*nif}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt);
    const Tomador comMotivo = Tomador::montar(IdentificacaoPessoa{*motivo}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt);
    QVERIFY(std::holds_alternative<Nif>(comNif.identificacao()));
    QVERIFY(std::holds_alternative<MotivoNaoNif>(comMotivo.identificacao()));
}

void TstPessoas::nifComMaisDeQuarentaCaracteresRejeitado()
{
    QCOMPARE(teste::codigoDoErro(Nif::de(QString(41, QChar{'A'}), "toma/NIF")), QStringLiteral("texto.tamanho"));
}

void TstPessoas::nomeNoLimiteDeTrezentosAceito()
{
    QVERIFY(NomeRazaoSocial::de(QString(300, QChar{'a'}), "toma/xNome").has_value());
}

void TstPessoas::nomeComMaisDeTrezentosRejeitado()
{
    QCOMPARE(teste::codigoDoErro(NomeRazaoSocial::de(QString(301, QChar{'a'}), "toma/xNome")), QStringLiteral("texto.tamanho"));
}

void TstPessoas::inscricaoMunicipalAteQuinzeCaracteres()
{
    QVERIFY(InscricaoMunicipal::de(QString(15, QChar{'1'}), "prest/IM").has_value());
    QCOMPARE(teste::codigoDoErro(InscricaoMunicipal::de(QString(16, QChar{'1'}), "prest/IM")), QStringLiteral("texto.tamanho"));
}

void TstPessoas::nomeVazioDoTomadorDevolveErroComCodigo()
{
    // Ausência de xNome é aceita (0-1). Um nome presente e vazio é erro de tamanho: devolve Erro com código e campo, não lança nem aborta.
    const auto nome = NomeRazaoSocial::de(QString{}, "toma/xNome");
    QVERIFY(!nome.has_value());
    QCOMPARE(teste::codigoDoErro(nome), QStringLiteral("texto.tamanho"));
    QCOMPARE(QString::fromStdString(nome.error().campo().value_or(std::string{})), QStringLiteral("toma/xNome"));
}

void TstPessoas::emailAteOitentaCaracteres()
{
    QVERIFY(EmailPessoa::de(QString(80, QChar{'a'}), "toma/email").has_value());
    QCOMPARE(teste::codigoDoErro(EmailPessoa::de(QString(81, QChar{'a'}), "toma/email")), QStringLiteral("texto.tamanho"));
}

QTEST_GUILESS_MAIN(TstPessoas)
#include "tst_pessoas.moc"

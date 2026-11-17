// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Classe da falha de envio (NL-3.C, plano 1.5 e 4.3): a regra motivo x causa x código E, por tabela literal de 126
// combinações (7 motivos x 9 causas x código E presente ou ausente). Nenhuma combinação não listada sai como recusa.
#include <nl/mid/composicao/classe_falha_envio.h>

#include <QtTest>

#include <iterator>
#include <optional>
#include <string>

namespace bk = nl::back;
namespace cp = nl::mid::composicao;

namespace {

using M = bk::MotivoFalhaSefin;
using C = bk::CausaFalhaSefin;

struct Linha {
    M motivo;
    C causa;
    const char* semCodigoE;  // classe esperada sem código E ("sem classe" quando o caminho antigo decide)
    const char* comCodigoE;  // classe esperada com código E
};

// Tabela da seção 1.5 (C1 a C9 e "sem classe"). Ordem: motivo, depois causa na ordem do enum.
const Linha tabela[] = {
    // Transitoria: Rede é C3; Nenhuma, NaoGerada e SefinIndisponivel são C6; as outras causas não listadas são C4.
    {M::Transitoria, C::Nenhuma, "C6", "C6"},
    {M::Transitoria, C::CertificadoDoEmissorProvavel, "C4", "C4"},
    {M::Transitoria, C::ServidorNaoConfirmado, "C4", "C4"},
    {M::Transitoria, C::Rede, "C3", "C3"},
    {M::Transitoria, C::ConfiguracaoDaConexao, "C4", "C4"},
    {M::Transitoria, C::PreparacaoDoEnvio, "C4", "C4"},
    {M::Transitoria, C::RespostaSemCodigoValido, "C4", "C4"},
    {M::Transitoria, C::SefinIndisponivel, "C6", "C6"},
    {M::Transitoria, C::NaoGerada, "C6", "C6"},
    // DpsNaoGerada: sem código E é C4 em toda causa; com código E, caminho antigo (sefin.<E>, sem classe).
    {M::DpsNaoGerada, C::Nenhuma, "C4", "sem classe"},
    {M::DpsNaoGerada, C::CertificadoDoEmissorProvavel, "C4", "sem classe"},
    {M::DpsNaoGerada, C::ServidorNaoConfirmado, "C4", "sem classe"},
    {M::DpsNaoGerada, C::Rede, "C4", "sem classe"},
    {M::DpsNaoGerada, C::ConfiguracaoDaConexao, "C4", "sem classe"},
    {M::DpsNaoGerada, C::PreparacaoDoEnvio, "C4", "sem classe"},
    {M::DpsNaoGerada, C::RespostaSemCodigoValido, "C4", "sem classe"},
    {M::DpsNaoGerada, C::SefinIndisponivel, "C4", "sem classe"},
    {M::DpsNaoGerada, C::NaoGerada, "C4", "sem classe"},
    // DpsDuplicada: sempre sefin.dps_duplicada (saída 5), sem classe.
    {M::DpsDuplicada, C::Nenhuma, "sem classe", "sem classe"},
    {M::DpsDuplicada, C::CertificadoDoEmissorProvavel, "sem classe", "sem classe"},
    {M::DpsDuplicada, C::ServidorNaoConfirmado, "sem classe", "sem classe"},
    {M::DpsDuplicada, C::Rede, "sem classe", "sem classe"},
    {M::DpsDuplicada, C::ConfiguracaoDaConexao, "sem classe", "sem classe"},
    {M::DpsDuplicada, C::PreparacaoDoEnvio, "sem classe", "sem classe"},
    {M::DpsDuplicada, C::RespostaSemCodigoValido, "sem classe", "sem classe"},
    {M::DpsDuplicada, C::SefinIndisponivel, "sem classe", "sem classe"},
    {M::DpsDuplicada, C::NaoGerada, "sem classe", "sem classe"},
    // Rejeitada: sem código E, só Nenhuma e RespostaSemCodigoValido são C7b; com código E, caminho antigo.
    {M::Rejeitada, C::Nenhuma, "C7b", "sem classe"},
    {M::Rejeitada, C::CertificadoDoEmissorProvavel, "C4", "sem classe"},
    {M::Rejeitada, C::ServidorNaoConfirmado, "C4", "sem classe"},
    {M::Rejeitada, C::Rede, "C4", "sem classe"},
    {M::Rejeitada, C::ConfiguracaoDaConexao, "C4", "sem classe"},
    {M::Rejeitada, C::PreparacaoDoEnvio, "C4", "sem classe"},
    {M::Rejeitada, C::RespostaSemCodigoValido, "C7b", "sem classe"},
    {M::Rejeitada, C::SefinIndisponivel, "C4", "sem classe"},
    {M::Rejeitada, C::NaoGerada, "C4", "sem classe"},
    // Definitiva: Certificado é C1, ServidorNaoConfirmado é C2, Configuracao é C8, Preparacao é C7a, RespostaSemCodigo é C7b;
    // as demais causas não listadas são C4 (inclui Definitiva + Nenhuma e Definitiva + Rede, pelo R-5 do CISO).
    {M::Definitiva, C::Nenhuma, "C4", "C4"},
    {M::Definitiva, C::CertificadoDoEmissorProvavel, "C1", "C1"},
    {M::Definitiva, C::ServidorNaoConfirmado, "C2", "C2"},
    {M::Definitiva, C::Rede, "C4", "C4"},
    {M::Definitiva, C::ConfiguracaoDaConexao, "C8", "C8"},
    {M::Definitiva, C::PreparacaoDoEnvio, "C7a", "C7a"},
    {M::Definitiva, C::RespostaSemCodigoValido, "C7b", "C7b"},
    {M::Definitiva, C::SefinIndisponivel, "C4", "C4"},
    {M::Definitiva, C::NaoGerada, "C4", "C4"},
    // Indeterminado: a consulta não disse o que aconteceu; Certificado é C5, o resto é C4. Código E é ignorado.
    {M::Indeterminado, C::Nenhuma, "C4", "C4"},
    {M::Indeterminado, C::CertificadoDoEmissorProvavel, "C5", "C5"},
    {M::Indeterminado, C::ServidorNaoConfirmado, "C4", "C4"},
    {M::Indeterminado, C::Rede, "C4", "C4"},
    {M::Indeterminado, C::ConfiguracaoDaConexao, "C4", "C4"},
    {M::Indeterminado, C::PreparacaoDoEnvio, "C4", "C4"},
    {M::Indeterminado, C::RespostaSemCodigoValido, "C4", "C4"},
    {M::Indeterminado, C::SefinIndisponivel, "C4", "C4"},
    {M::Indeterminado, C::NaoGerada, "C4", "C4"},
    // ComparacaoIndisponivel: C9 em qualquer causa e com ou sem código E.
    {M::ComparacaoIndisponivel, C::Nenhuma, "C9", "C9"},
    {M::ComparacaoIndisponivel, C::CertificadoDoEmissorProvavel, "C9", "C9"},
    {M::ComparacaoIndisponivel, C::ServidorNaoConfirmado, "C9", "C9"},
    {M::ComparacaoIndisponivel, C::Rede, "C9", "C9"},
    {M::ComparacaoIndisponivel, C::ConfiguracaoDaConexao, "C9", "C9"},
    {M::ComparacaoIndisponivel, C::PreparacaoDoEnvio, "C9", "C9"},
    {M::ComparacaoIndisponivel, C::RespostaSemCodigoValido, "C9", "C9"},
    {M::ComparacaoIndisponivel, C::SefinIndisponivel, "C9", "C9"},
    {M::ComparacaoIndisponivel, C::NaoGerada, "C9", "C9"},
};

QString nomeDaClasse(const std::optional<cp::ClasseFalhaEnvio>& classe)
{
    if (!classe) {
        return QStringLiteral("sem classe");
    }
    switch (*classe) {
    case cp::ClasseFalhaEnvio::CertificadoProvavel:
        return QStringLiteral("C1");
    case cp::ClasseFalhaEnvio::ServidorNaoConfirmado:
        return QStringLiteral("C2");
    case cp::ClasseFalhaEnvio::RedeAntesDoEnvio:
        return QStringLiteral("C3");
    case cp::ClasseFalhaEnvio::EstadoIncerto:
        return QStringLiteral("C4");
    case cp::ClasseFalhaEnvio::EstadoIncertoCertificadoProvavel:
        return QStringLiteral("C5");
    case cp::ClasseFalhaEnvio::NaoConcluido:
        return QStringLiteral("C6");
    case cp::ClasseFalhaEnvio::FalhaAntesDoEnvio:
        return QStringLiteral("C7a");
    case cp::ClasseFalhaEnvio::RespostaSemCodigo:
        return QStringLiteral("C7b");
    case cp::ClasseFalhaEnvio::ConfiguracaoDaConexao:
        return QStringLiteral("C8");
    case cp::ClasseFalhaEnvio::ComparacaoIndisponivel:
        return QStringLiteral("C9");
    }
    return QStringLiteral("?");
}

bk::FalhaSefin falhaDe(M motivo, C causa, bool comCodigoE)
{
    return bk::FalhaSefin{motivo, comCodigoE ? std::string("E1207") : std::string(), {}, causa};
}

} // namespace

class TesteClasseFalhaEnvio : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void tabelaInteiraDeMotivoCausaECodigo();
    void combinacaoImpossivelViraEstadoIncerto();
    void soRecusaComCodigoEDpsDuplicadaFicamSemClasse();
};

void TesteClasseFalhaEnvio::initTestCase()
{
}

void TesteClasseFalhaEnvio::cleanupTestCase()
{
}

void TesteClasseFalhaEnvio::tabelaInteiraDeMotivoCausaECodigo()
{
    // 7 motivos x 9 causas = 63 linhas; cada linha confere com e sem código E: 126 asserções.
    QCOMPARE(static_cast<int>(std::size(tabela)), 63);
    int conferidas = 0;
    for (const Linha& linha : tabela) {
        for (bool comCodigoE : {false, true}) {
            const bk::FalhaSefin falha = falhaDe(linha.motivo, linha.causa, comCodigoE);
            const char* esperada = comCodigoE ? linha.comCodigoE : linha.semCodigoE;
            QCOMPARE(nomeDaClasse(cp::classeDaFalha(falha)), QString::fromLatin1(esperada));
            ++conferidas;
        }
    }
    QCOMPARE(conferidas, 126);
}

void TesteClasseFalhaEnvio::combinacaoImpossivelViraEstadoIncerto()
{
    // Combinações fora da tabela da seção 1.5 caem em C4, a conservadora: nunca mandam usar outro número.
    QCOMPARE(nomeDaClasse(cp::classeDaFalha(falhaDe(M::Transitoria, C::CertificadoDoEmissorProvavel, false))), QStringLiteral("C4"));
    QCOMPARE(nomeDaClasse(cp::classeDaFalha(falhaDe(M::Definitiva, C::Nenhuma, false))), QStringLiteral("C4"));
    QCOMPARE(nomeDaClasse(cp::classeDaFalha(falhaDe(M::Definitiva, C::Rede, false))), QStringLiteral("C4"));
    QCOMPARE(nomeDaClasse(cp::classeDaFalha(falhaDe(M::DpsNaoGerada, C::Nenhuma, false))), QStringLiteral("C4"));
}

void TesteClasseFalhaEnvio::soRecusaComCodigoEDpsDuplicadaFicamSemClasse()
{
    // Sem classe só em DpsDuplicada (qualquer causa) e em Rejeitada ou DpsNaoGerada com código E. Nenhum Indeterminado sai sem classe.
    const M motivos[] = {M::Transitoria, M::DpsNaoGerada, M::DpsDuplicada, M::Rejeitada, M::Definitiva, M::Indeterminado,
                         M::ComparacaoIndisponivel};
    const C causas[] = {C::Nenhuma, C::CertificadoDoEmissorProvavel, C::ServidorNaoConfirmado, C::Rede, C::ConfiguracaoDaConexao,
                        C::PreparacaoDoEnvio, C::RespostaSemCodigoValido, C::SefinIndisponivel, C::NaoGerada};
    for (M motivo : motivos) {
        for (C causa : causas) {
            for (bool comCodigoE : {false, true}) {
                const bool semClasseEsperada = motivo == M::DpsDuplicada
                                               || ((motivo == M::Rejeitada || motivo == M::DpsNaoGerada) && comCodigoE);
                const bool temClasse = cp::classeDaFalha(falhaDe(motivo, causa, comCodigoE)).has_value();
                QVERIFY2(temClasse == !semClasseEsperada, "classe fora da regra de nulo");
            }
        }
    }
}

QTEST_APPLESS_MAIN(TesteClasseFalhaEnvio)
#include "tst_classe_falha_envio.moc"

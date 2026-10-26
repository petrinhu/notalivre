// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes unitários do Numerador da DPS (NL-1.3). Regra pura: o repositório é um falso em memória, só nesta suíte.
// Invariante nomeado: reservar devolve sempre número maior que qualquer número reservado, confirmado ou pulado
// conhecido pelo port. O Numerador não trava nada; a exclusão mútua entre processos é do adaptador (NL-3.5/3.7),
// e o falso aqui recusa gravação duplicada com numeracao.conflito para provar que o domínio propaga essa recusa.

#include <nl/back/numeracao/numerador.h>

#include <cstdint>
#include <map>
#include <optional>
#include <utility>

#include <QtTest>

#include <nl/back/dps/chave_acesso.h>
#include <nl/back/dps/numero_dps.h>
#include <nl/back/dps/serie.h>
#include <nl/back/numeracao/estado_numero.h>
#include <nl/back/numeracao/motivo_pulo.h>
#include <nl/back/numeracao/reserva.h>
#include <nl/back/ports/repositorio_numeracao.h>

using nl::back::ChaveAcesso;
using nl::back::EstadoNumero;
using nl::back::MotivoPulo;
using nl::back::NumeroDps;
using nl::back::Numerador;
using nl::back::RepositorioNumeracao;
using nl::back::Reserva;
using nl::back::Serie;
using nl::base::Erro;

namespace {

// Repositório falso: guarda reservas por (série, número) e expõe as falhas para os testes provarem a propagação.
class RepositorioFalso final : public RepositorioNumeracao
{
public:
    struct Registro
    {
        EstadoNumero estado;
        QString motivo;
        QString chave;
    };

    // Semeia uma reserva já existente no armazenamento, no estado dado.
    void semear(const Reserva& reserva, EstadoNumero estado)
    {
        registros_[chaveDe(reserva)] = Registro{estado, {}, {}};
    }

    const Registro* registro(const Reserva& reserva) const
    {
        const auto achado = registros_.find(chaveDe(reserva));
        return achado == registros_.end() ? nullptr : &achado->second;
    }

    std::size_t total() const { return registros_.size(); }

    // Simula corrida: ultimoReservado devolve vazio mesmo com reservas gravadas (outro processo gravou depois).
    bool ultimoMentiroso = false;
    bool falharLeitura = false;
    bool falharGravacao = false;

    std::expected<std::optional<NumeroDps>, Erro> ultimoReservado(const Serie& serie) override
    {
        if (falharLeitura) {
            return std::unexpected(Erro{"repositorio.indisponivel", std::nullopt, "banco indisponível"});
        }
        if (ultimoMentiroso) {
            return std::optional<NumeroDps>{};
        }
        std::optional<std::int64_t> maior;
        for (const auto& entrada : registros_) {
            if (entrada.first.first == serie.texto() && (!maior || entrada.first.second > *maior)) {
                maior = entrada.first.second;
            }
        }
        if (!maior) {
            return std::optional<NumeroDps>{};
        }
        return std::optional<NumeroDps>{*NumeroDps::de(*maior)};
    }

    std::expected<std::optional<EstadoNumero>, Erro> estadoDe(const Reserva& reserva) override
    {
        const auto achado = registros_.find(chaveDe(reserva));
        if (achado == registros_.end()) {
            return std::optional<EstadoNumero>{};
        }
        return std::optional<EstadoNumero>{achado->second.estado};
    }

    std::expected<void, Erro> gravarReserva(const Reserva& reserva) override
    {
        if (falharGravacao) {
            return std::unexpected(Erro{"repositorio.indisponivel", std::nullopt, "banco indisponível"});
        }
        if (registros_.contains(chaveDe(reserva))) {
            return std::unexpected(Erro{"numeracao.conflito", std::nullopt, "número já reservado nesta série"});
        }
        registros_[chaveDe(reserva)] = Registro{EstadoNumero::Reservado, {}, {}};
        return {};
    }

    std::expected<void, Erro> gravarConfirmacao(const Reserva& reserva, const ChaveAcesso& chave) override
    {
        const auto achado = registros_.find(chaveDe(reserva));
        if (achado == registros_.end()) {
            return std::unexpected(Erro{"repositorio.registro_ausente", std::nullopt, "reserva inexistente"});
        }
        achado->second = Registro{EstadoNumero::Confirmado, {}, chave.texto()};
        return {};
    }

    std::expected<void, Erro> gravarPulo(const Reserva& reserva, const MotivoPulo& motivo) override
    {
        const auto achado = registros_.find(chaveDe(reserva));
        if (achado == registros_.end()) {
            return std::unexpected(Erro{"repositorio.registro_ausente", std::nullopt, "reserva inexistente"});
        }
        achado->second = Registro{EstadoNumero::Pulado, motivo.texto(), {}};
        return {};
    }

private:
    static std::pair<QString, std::int64_t> chaveDe(const Reserva& reserva)
    {
        return {reserva.serie().texto(), reserva.numero().valor()};
    }

    std::map<std::pair<QString, std::int64_t>, Registro> registros_;
};

Serie serieDe(int valor)
{
    return *Serie::de(valor);
}

Reserva reservaDe(int serie, std::int64_t numero)
{
    return Reserva{serieDe(serie), *NumeroDps::de(numero)};
}

ChaveAcesso chaveValida()
{
    return *ChaveAcesso::de(u"123456789" u"12ABC34501DE35" u"123456789012345678901234567");
}

MotivoPulo motivoValido()
{
    return *MotivoPulo::de(u"Falha antes do envio");
}

QString codigo(const Erro& erro)
{
    return QString::fromStdString(erro.codigo());
}

} // namespace

class TstNumerador : public QObject
{
    Q_OBJECT

private slots:
    void primeiraReservaEhNumeroUm();
    void reservaSeguinteNaoRepeteReservada();
    void numeroReservadoNuncaReutilizado();
    void numeroConfirmadoNuncaReutilizado();
    void numeroPuladoNuncaReutilizado();
    void puloGravaMotivo();
    void confirmarSemReservaFalha();
    void confirmarDuasVezesFalha();
    void puloDepoisDeConfirmadoFalha();
    void reservaDeOutraSerieFalha();
    void esgotamentoNaoTransborda();
    void conflitoDeGravacaoPropagado();
    void erroDeLeituraPropagadoComContexto();
    void gravacaoFalhaNaoEntregaReserva();
};

void TstNumerador::primeiraReservaEhNumeroUm()
{
    RepositorioFalso repositorio;
    Numerador numerador{serieDe(1), repositorio};
    const auto reserva = numerador.reservar();
    QVERIFY(reserva.has_value());
    QCOMPARE(reserva->numero().texto(), QStringLiteral("000000000000001"));
    QCOMPARE(repositorio.total(), std::size_t{1});
}

void TstNumerador::reservaSeguinteNaoRepeteReservada()
{
    RepositorioFalso repositorio;
    Numerador numerador{serieDe(1), repositorio};
    const auto primeira = numerador.reservar();
    QVERIFY(primeira.has_value());
    const auto segunda = numerador.reservar();
    QVERIFY(segunda.has_value());
    const auto terceira = numerador.reservar();
    QVERIFY(terceira.has_value());
    QCOMPARE(primeira->numero().texto(), QStringLiteral("000000000000001"));
    QCOMPARE(segunda->numero().texto(), QStringLiteral("000000000000002"));
    QCOMPARE(terceira->numero().texto(), QStringLiteral("000000000000003"));
}

void TstNumerador::numeroReservadoNuncaReutilizado()
{
    // Plano, NL-1.3: numeroReservadoNuncaReutilizado. Semeia três estados (reservado, confirmado e pulado)
    // e exige que duas reservas seguintes saiam distintas e acima de 3.
    RepositorioFalso repositorio;
    repositorio.semear(reservaDe(1, 1), EstadoNumero::Reservado);
    repositorio.semear(reservaDe(1, 2), EstadoNumero::Confirmado);
    repositorio.semear(reservaDe(1, 3), EstadoNumero::Pulado);
    Numerador numerador{serieDe(1), repositorio};
    const auto quarta = numerador.reservar();
    QVERIFY(quarta.has_value());
    const auto quinta = numerador.reservar();
    QVERIFY(quinta.has_value());
    QCOMPARE(quarta->numero().texto(), QStringLiteral("000000000000004"));
    QCOMPARE(quinta->numero().texto(), QStringLiteral("000000000000005"));
}

void TstNumerador::numeroConfirmadoNuncaReutilizado()
{
    RepositorioFalso repositorio;
    Numerador numerador{serieDe(1), repositorio};
    const auto primeira = numerador.reservar();
    QVERIFY(primeira.has_value());
    const auto confirmada = numerador.confirmar(*primeira, chaveValida());
    QVERIFY(confirmada.has_value());
    const auto segunda = numerador.reservar();
    QVERIFY(segunda.has_value());
    QCOMPARE(segunda->numero().texto(), QStringLiteral("000000000000002"));
    const auto* registro = repositorio.registro(*primeira);
    QVERIFY(registro != nullptr);
    QCOMPARE(registro->estado, EstadoNumero::Confirmado);
    QCOMPARE(registro->chave, chaveValida().texto());
}

void TstNumerador::numeroPuladoNuncaReutilizado()
{
    RepositorioFalso repositorio;
    Numerador numerador{serieDe(1), repositorio};
    const auto primeira = numerador.reservar();
    QVERIFY(primeira.has_value());
    const auto pulada = numerador.pular(*primeira, motivoValido());
    QVERIFY(pulada.has_value());
    const auto segunda = numerador.reservar();
    QVERIFY(segunda.has_value());
    QCOMPARE(segunda->numero().texto(), QStringLiteral("000000000000002"));
}

void TstNumerador::puloGravaMotivo()
{
    // Plano, NL-1.3: puloSempreTemMotivo. O motivo entra no armazenamento junto com o pulo.
    RepositorioFalso repositorio;
    Numerador numerador{serieDe(1), repositorio};
    const auto primeira = numerador.reservar();
    QVERIFY(primeira.has_value());
    const auto pulada = numerador.pular(*primeira, motivoValido());
    QVERIFY(pulada.has_value());
    const auto* registro = repositorio.registro(*primeira);
    QVERIFY(registro != nullptr);
    QCOMPARE(registro->estado, EstadoNumero::Pulado);
    QCOMPARE(registro->motivo, QStringLiteral("Falha antes do envio"));
}

void TstNumerador::confirmarSemReservaFalha()
{
    // Número 7 nunca foi reservado: confirmar não pode criar a reserva por baixo dos panos.
    RepositorioFalso repositorio;
    Numerador numerador{serieDe(1), repositorio};
    const auto confirmada = numerador.confirmar(reservaDe(1, 7), chaveValida());
    QVERIFY(!confirmada.has_value());
    QCOMPARE(codigo(confirmada.error()), QStringLiteral("numeracao.estado_invalido"));
    QCOMPARE(repositorio.total(), std::size_t{0});
}

void TstNumerador::confirmarDuasVezesFalha()
{
    RepositorioFalso repositorio;
    Numerador numerador{serieDe(1), repositorio};
    const auto primeira = numerador.reservar();
    QVERIFY(primeira.has_value());
    const auto ok = numerador.confirmar(*primeira, chaveValida());
    QVERIFY(ok.has_value());
    const auto repetida = numerador.confirmar(*primeira, chaveValida());
    QVERIFY(!repetida.has_value());
    QCOMPARE(codigo(repetida.error()), QStringLiteral("numeracao.estado_invalido"));
}

void TstNumerador::puloDepoisDeConfirmadoFalha()
{
    RepositorioFalso repositorio;
    Numerador numerador{serieDe(1), repositorio};
    const auto primeira = numerador.reservar();
    QVERIFY(primeira.has_value());
    const auto ok = numerador.confirmar(*primeira, chaveValida());
    QVERIFY(ok.has_value());
    const auto pulada = numerador.pular(*primeira, motivoValido());
    QVERIFY(!pulada.has_value());
    QCOMPARE(codigo(pulada.error()), QStringLiteral("numeracao.estado_invalido"));
}

void TstNumerador::reservaDeOutraSerieFalha()
{
    RepositorioFalso repositorio;
    Numerador serieUm{serieDe(1), repositorio};
    Numerador serieDois{serieDe(2), repositorio};
    const auto primeira = serieUm.reservar();
    QVERIFY(primeira.has_value());
    const auto cruzada = serieDois.confirmar(*primeira, chaveValida());
    QVERIFY(!cruzada.has_value());
    QCOMPARE(codigo(cruzada.error()), QStringLiteral("numeracao.serie_divergente"));
}

void TstNumerador::esgotamentoNaoTransborda()
{
    // Último número da faixa de quinze posições já usado: a próxima reserva não pode passar de 999999999999999.
    RepositorioFalso repositorio;
    repositorio.semear(reservaDe(1, 999999999999999), EstadoNumero::Confirmado);
    Numerador numerador{serieDe(1), repositorio};
    const auto proxima = numerador.reservar();
    QVERIFY(!proxima.has_value());
    QCOMPARE(codigo(proxima.error()), QStringLiteral("numeracao.esgotada"));
    QCOMPARE(repositorio.total(), std::size_t{1});
}

void TstNumerador::conflitoDeGravacaoPropagado()
{
    // Barreira entre processos: a leitura mente (vazia), o número 1 já existe, e a gravação recusa.
    // O domínio não pode engolir a recusa nem devolver uma reserva que não foi gravada.
    RepositorioFalso repositorio;
    repositorio.semear(reservaDe(1, 1), EstadoNumero::Reservado);
    repositorio.ultimoMentiroso = true;
    Numerador numerador{serieDe(1), repositorio};
    const auto reserva = numerador.reservar();
    QVERIFY(!reserva.has_value());
    QCOMPARE(codigo(reserva.error()), QStringLiteral("numeracao.conflito"));
    QCOMPARE(repositorio.total(), std::size_t{1});
}

void TstNumerador::erroDeLeituraPropagadoComContexto()
{
    // Preserva a causa (código do repositório) e acrescenta o que estava sendo feito na mensagem.
    RepositorioFalso repositorio;
    repositorio.falharLeitura = true;
    Numerador numerador{serieDe(1), repositorio};
    const auto reserva = numerador.reservar();
    QVERIFY(!reserva.has_value());
    QCOMPARE(codigo(reserva.error()), QStringLiteral("repositorio.indisponivel"));
    QVERIFY(QString::fromStdString(reserva.error().mensagem()).contains(QStringLiteral("reservar")));
}

void TstNumerador::gravacaoFalhaNaoEntregaReserva()
{
    RepositorioFalso repositorio;
    repositorio.falharGravacao = true;
    Numerador numerador{serieDe(1), repositorio};
    const auto reserva = numerador.reservar();
    QVERIFY(!reserva.has_value());
    QCOMPARE(codigo(reserva.error()), QStringLiteral("repositorio.indisponivel"));
    QCOMPARE(repositorio.total(), std::size_t{0});
}

QTEST_GUILESS_MAIN(TstNumerador)
#include "tst_numerador.moc"

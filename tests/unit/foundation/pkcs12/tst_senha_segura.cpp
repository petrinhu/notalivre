// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do interior da SenhaSegura com libsodium (NL-3.2c, ADR-0002). Um slot por nome no CTest (pkcs12.senha.<nome>).
// Senha sintética, sem certificado nem fixture. A falha de alocação e a de sodium_init entram pelo
// AcessoDeTesteSenhaSegura, com ponteiros de função passados na chamada; nenhuma variável global do programa muda.
// "Sem memória comum" é afirmado pelo contador do alocador de teste: no caminho de falha, o alocador de produção
// (sodium_malloc) não é chamado. Este teste não prova que o OpenSSL ou o libc não copiem a senha por fora.

#include <nl/foundation/pkcs12/senha_segura.h>

#include <QByteArray>
#include <QByteArrayView>
#include <QtTest>

#include <cstddef>
#include <cstring>
#include <new>
#include <string>

#include "acesso_teste_senha.h"

namespace {

using nl::foundation::pkcs12::AcessoDeTesteSenhaSegura;
using nl::foundation::pkcs12::SenhaSegura;

// Contador do alocador de teste. Vive só no executável de teste; o programa não tem como trocá-lo.
int gAlocacoes = 0;

void* alocaSemMemoria(std::size_t) noexcept
{
    ++gAlocacoes;
    return nullptr;
}

bool bibliotecaPronta() noexcept
{
    return true;
}

bool bibliotecaFalha() noexcept
{
    return false;
}

} // namespace

class TesteSenhaSegura : public QObject {
    Q_OBJECT

private slots:
    void tentarDevolveErroSemMemoriaComum();
    void deLancaEmFalhaDeAlocacao();
    void senhaVaziaFunciona();
    void comoCTerminaEmNul();
    void sodiumInitFalhoNaoAlocaEmMemoriaComum();
};

// Alocador sem memória: tentar() tem de devolver erro, não senha, e tem de ter pedido a memória uma vez.
void TesteSenhaSegura::tentarDevolveErroSemMemoriaComum()
{
    gAlocacoes = 0;
    auto resultado = AcessoDeTesteSenhaSegura::tentarComFalhas(QByteArrayLiteral("segredo"), &alocaSemMemoria,
                                                               &bibliotecaPronta);
    QVERIFY2(!resultado.has_value(), "tentar() devolveu senha com o alocador sem memória");
    QCOMPARE(gAlocacoes, 1);
    QVERIFY2(resultado.error().codigo() == "senha.sem_memoria_segura",
             "código de erro inesperado para falha de alocação");
}

// de() não devolve erro: a mesma falha tem de sair como std::bad_alloc, como o new de antes. Vale também para sodium_init falho.
void TesteSenhaSegura::deLancaEmFalhaDeAlocacao()
{
    bool lancouPorAlocacao = false;
    try {
        static_cast<void>(AcessoDeTesteSenhaSegura::deComFalhas(QByteArrayLiteral("segredo"), &alocaSemMemoria,
                                                                &bibliotecaPronta));
    } catch (const std::bad_alloc&) {
        lancouPorAlocacao = true;
    }
    QVERIFY2(lancouPorAlocacao, "de() não lançou std::bad_alloc com o alocador sem memória");

    bool lancouPorInit = false;
    try {
        static_cast<void>(AcessoDeTesteSenhaSegura::deComFalhas(QByteArrayLiteral("segredo"), &alocaSemMemoria,
                                                                &bibliotecaFalha));
    } catch (const std::bad_alloc&) {
        lancouPorInit = true;
    }
    QVERIFY2(lancouPorInit, "de() não lançou std::bad_alloc com sodium_init falho");
}

// Senha vazia: a região existe (capacidade 1, só o NUL), o tamanho é zero e o ponteiro é NUL-terminado.
void TesteSenhaSegura::senhaVaziaFunciona()
{
    auto resultado = SenhaSegura::tentar(QByteArray());
    QVERIFY2(resultado.has_value(), "tentar() falhou com senha vazia");
    QCOMPARE(resultado->tamanho(), std::size_t{0});
    QVERIFY(resultado->comoC() != nullptr);
    QCOMPARE(resultado->comoC()[0], '\0');
}

// O interior tem de terminar em NUL logo depois dos bytes úteis, mesmo que o alocador entregue lixo.
void TesteSenhaSegura::comoCTerminaEmNul()
{
    auto resultado = SenhaSegura::tentar(QByteArrayLiteral("abc"));
    QVERIFY2(resultado.has_value(), "tentar() falhou com senha curta");
    QCOMPARE(resultado->tamanho(), std::size_t{3});
    QVERIFY(std::memcmp(resultado->comoC(), "abc", 3) == 0);
    QCOMPARE(resultado->comoC()[3], '\0');
}

// Sem sodium_init não há alocação: a falha da biblioteca vale como falha de alocação e não cai na memória comum.
void TesteSenhaSegura::sodiumInitFalhoNaoAlocaEmMemoriaComum()
{
    gAlocacoes = 0;
    auto resultado = AcessoDeTesteSenhaSegura::tentarComFalhas(QByteArrayLiteral("segredo"), &alocaSemMemoria,
                                                               &bibliotecaFalha);
    QVERIFY2(!resultado.has_value(), "tentar() devolveu senha com sodium_init falho");
    QCOMPARE(gAlocacoes, 0);
}

QTEST_APPLESS_MAIN(TesteSenhaSegura)
#include "tst_senha_segura.moc"

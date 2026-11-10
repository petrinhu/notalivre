// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Testes do interior da SenhaSegura com libsodium (NL-3.2c, ADR-0002). Um slot por nome no CTest (pkcs12.senha.<nome>).
// Senha sintética, sem certificado nem fixture. A falha de alocação e a de sodium_init entram pelo
// AcessoDeTesteSenhaSegura, com ponteiros de função passados na chamada; nenhuma variável global do programa muda.
// "Sem memória comum" é afirmado pelo contador do alocador de teste: no caminho de falha, o alocador de produção
// (sodium_malloc) não é chamado. Este teste não prova que o OpenSSL ou o libc não copiem a senha por fora.
// Região fora do core dump (regiaoForaDoCoreDump): o dd do VmFlags vem do interior do sodium_malloc, não da chamada de
// senha_segura.cpp:86, cujo madvise é nulo porque o endereço do usuário não está alinhado (man 2 madvise). A parte (b)
// tem uma direção só: lo presente implica bloqueadaEmMemoria() verdadeiro. A igualdade é proibida, porque sob sanitizer
// o mlock é interceptado e devolve sucesso sem travar (sanitizer_common_interceptors), e lo some na produção boa.
// Risco residual aceito (CISO, 07/11/2026): canário e página de guarda anterior não têm teste aqui; são propriedade
// documentada do libsodium, e um alocador imitador (mmap à mão) passaria. Essa troca aparece na leitura do blob (L-12).

#include <nl/foundation/pkcs12/senha_segura.h>

#include <QByteArray>
#include <QByteArrayView>
#include <QtTest>

#include <sodium.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <new>
#include <sstream>
#include <string>
#include <vector>

#include "acesso_teste_senha.h"

namespace {

using nl::foundation::pkcs12::AcessoDeTesteSenhaSegura;
using nl::foundation::pkcs12::SenhaSegura;

// Sonda de leitura sem SIGSEGV: write() de 1 byte para um pipe devolve -1 (EFAULT) quando o endereço não é legível.
// Ler o endereço diretamente derrubaria o processo na página de guarda; a sonda só pergunta ao kernel.
class SondaDeLeitura {
public:
    SondaDeLeitura()
    {
        valida_ = pipe(fds_) == 0;
    }
    ~SondaDeLeitura()
    {
        if (valida_) {
            close(fds_[0]);
            close(fds_[1]);
        }
    }
    SondaDeLeitura(const SondaDeLeitura&) = delete;
    SondaDeLeitura& operator=(const SondaDeLeitura&) = delete;

    bool valida() const noexcept { return valida_; }

    // Verdadeiro se o kernel copia 1 byte do endereço. O byte escrito é descartado logo em seguida.
    bool legivel(std::uintptr_t endereco) const noexcept
    {
        if (write(fds_[1], reinterpret_cast<const void*>(endereco), 1) != 1) {
            return false;
        }
        char descarte = 0;
        // O write já provou a leitura; este read só drena o pipe.
        [[maybe_unused]] const ssize_t drenado = read(fds_[0], &descarte, 1);
        return true;
    }

private:
    int fds_[2]{-1, -1};
    bool valida_{false};
};

// Primeira página, a partir de `inicio` alinhado para cima, que a sonda não consegue ler; 0 se não houver nas `paginas`.
std::uintptr_t primeiraPaginaIlegivel(const SondaDeLeitura& sonda, std::uintptr_t inicio, std::size_t tamanhoPagina,
                                      std::size_t paginas)
{
    const std::uintptr_t primeira = (inicio + tamanhoPagina - 1) / tamanhoPagina * tamanhoPagina;
    for (std::size_t i = 0; i < paginas; ++i) {
        const std::uintptr_t pagina = primeira + i * tamanhoPagina;
        if (!sonda.legivel(pagina)) {
            return pagina;
        }
    }
    return 0;
}

// Cabeçalho de faixa do smaps: "inicio-fim perms offset dev inode caminho". Linha de campo ("VmFlags:", "Size:") tem ":".
bool faixaDoCabecalho(const std::string& linha, std::uintptr_t& inicio, std::uintptr_t& fim)
{
    const std::string primeiro = linha.substr(0, linha.find(' '));
    const auto traco = primeiro.find('-');
    if (traco == std::string::npos || primeiro.find(':') != std::string::npos) {
        return false;
    }
    inicio = std::strtoull(primeiro.substr(0, traco).c_str(), nullptr, 16);
    fim = std::strtoull(primeiro.substr(traco + 1).c_str(), nullptr, 16);
    return true;
}

enum class EstadoSmaps { Ok, Ilegivel, FaixaNaoAchada, SemVmFlags };

struct FlagsDaRegiao {
    EstadoSmaps estado{EstadoSmaps::FaixaNaoAchada};
    std::vector<std::string> palavras;
};

// Lê /proc/self/smaps e devolve as palavras de VmFlags da faixa que contém o endereço. Qualquer lacuna é estado, não pulo.
FlagsDaRegiao flagsDaRegiao(std::uintptr_t endereco)
{
    FlagsDaRegiao regiao;
    std::ifstream smaps("/proc/self/smaps");
    if (!smaps) {
        regiao.estado = EstadoSmaps::Ilegivel;
        return regiao;
    }
    bool dentro = false;
    bool achou = false;
    std::string linha;
    while (std::getline(smaps, linha)) {
        std::uintptr_t inicio = 0;
        std::uintptr_t fim = 0;
        if (faixaDoCabecalho(linha, inicio, fim)) {
            dentro = inicio <= endereco && endereco < fim;
            achou = achou || dentro;
            continue;
        }
        if (dentro && linha.rfind("VmFlags:", 0) == 0) {
            std::istringstream campos(linha.substr(std::string("VmFlags:").size()));
            std::string palavra;
            while (campos >> palavra) {
                regiao.palavras.push_back(palavra);
            }
            regiao.estado = EstadoSmaps::Ok;
            return regiao;
        }
    }
    regiao.estado = achou ? EstadoSmaps::SemVmFlags : EstadoSmaps::FaixaNaoAchada;
    return regiao;
}

bool contemPalavra(const std::vector<std::string>& palavras, const std::string& alvo)
{
    for (const auto& palavra : palavras) {
        if (palavra == alvo) {
            return true;
        }
    }
    return false;
}

struct ConferenciaDaRegiao {
    std::string motivo; // vazio quando a região passa
    bool dd{false};
    bool lo{false};
};

// dd (fora do core dump) é obrigatório. lo (travada) só exige bloqueadaEmMemoria() verdadeiro: a direção é esta, não a igualdade.
ConferenciaDaRegiao conferirRegiaoDaSenha(const SenhaSegura& senha)
{
    ConferenciaDaRegiao conferencia;
    const auto endereco = reinterpret_cast<std::uintptr_t>(senha.comoC());
    const FlagsDaRegiao regiao = flagsDaRegiao(endereco);
    switch (regiao.estado) {
    case EstadoSmaps::Ilegivel:
        conferencia.motivo = "/proc/self/smaps ilegível";
        return conferencia;
    case EstadoSmaps::FaixaNaoAchada:
        conferencia.motivo = "faixa da senha não achada em /proc/self/smaps";
        return conferencia;
    case EstadoSmaps::SemVmFlags:
        conferencia.motivo = "VmFlags ausente na faixa da senha";
        return conferencia;
    case EstadoSmaps::Ok:
        break;
    }
    conferencia.dd = contemPalavra(regiao.palavras, "dd");
    conferencia.lo = contemPalavra(regiao.palavras, "lo");
    if (!conferencia.dd) {
        conferencia.motivo = "VmFlags da região sem dd: a senha pode entrar no core dump";
    } else if (conferencia.lo && !senha.bloqueadaEmMemoria()) {
        conferencia.motivo = "região travada pelo kernel (lo) mas bloqueadaEmMemoria() é falso";
    }
    return conferencia;
}

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
    void bufferTemPaginaDeGuardaDepois();
    void tentarInicializaOSodium();
    void regiaoForaDoCoreDump();
    void deInicializaOSodium();
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

// Propriedade do sodium_malloc (doc memory_management): a região termina no fim de uma página e é seguida de página de
// guarda, sem permissão. Um buffer de new não tem essa página logo depois, por isso este teste mata a troca por new (M2b).
// Três tamanhos: um, um que deixa folga de alinhamento e um maior. A sonda confere, por páginas, a primeira ilegível.
void TesteSenhaSegura::bufferTemPaginaDeGuardaDepois()
{
    SondaDeLeitura sonda;
    QVERIFY2(sonda.valida(), "pipe() falhou: sem sonda de leitura");
    const auto tamanhoPagina = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
    QVERIFY2(tamanhoPagina > 0, "sysconf(_SC_PAGESIZE) inválido");
    for (const std::size_t tamanho : {std::size_t{1}, std::size_t{17}, std::size_t{100}}) {
        const QByteArray bytes(static_cast<qsizetype>(tamanho), 'x');
        auto resultado = SenhaSegura::tentar(bytes);
        QVERIFY2(resultado.has_value(), "tentar() falhou ao criar a senha do teste");
        const auto fim = reinterpret_cast<std::uintptr_t>(resultado->comoC()) + tamanho + 1; // capacidade = tamanho + NUL
        const auto guarda = primeiraPaginaIlegivel(sonda, fim, tamanhoPagina, 2);
        QVERIFY2(guarda != 0, "nenhuma página ilegível logo após o buffer: a região não é a do sodium_malloc");
        QVERIFY2(guarda - fim < tamanhoPagina, "a página ilegível está a mais de uma página do fim do buffer");
        qInfo() << "tamanho" << tamanho << "- página de guarda a" << (guarda - fim) << "bytes do fim do buffer";
    }
}

// Prova de que tentar() chama sodium_init antes do primeiro sodium_malloc (doc usage: sodium_init devolve 0 na primeira
// vez e 1 depois). Slot em processo próprio: se a produção não inicializa, tentar() aborta antes desta asserção.
void TesteSenhaSegura::tentarInicializaOSodium()
{
    auto resultado = SenhaSegura::tentar(QByteArrayLiteral("segredo"));
    QVERIFY2(resultado.has_value(), "tentar() falhou ao criar a senha do teste");
    QCOMPARE(sodium_init(), 1);
}

// Região da senha fora do core dump (C1.9, modelo-de-ameacas.md:69): para tentar() e de(), em três tamanhos, a faixa de
// /proc/self/smaps que contém comoC() tem dd em VmFlags, e lo implica bloqueadaEmMemoria(). Ilegível, faixa não achada ou
// VmFlags ausente são falha, nunca QSKIP. Mata M2d (mmap, sem madvise), que não tem dd.
void TesteSenhaSegura::regiaoForaDoCoreDump()
{
    for (const std::size_t tamanho : {std::size_t{1}, std::size_t{17}, std::size_t{100}}) {
        const QByteArray bytes(static_cast<qsizetype>(tamanho), 'x');

        auto porTentar = SenhaSegura::tentar(bytes);
        QVERIFY2(porTentar.has_value(), "tentar() falhou ao criar a senha do teste");
        const ConferenciaDaRegiao conferidaTentar = conferirRegiaoDaSenha(*porTentar);
        const std::string rotuloTentar = "tentar, tamanho " + std::to_string(tamanho) + ": " + conferidaTentar.motivo;
        QVERIFY2(conferidaTentar.motivo.empty(), rotuloTentar.c_str());
        qInfo() << "tentar tamanho" << tamanho << "dd" << conferidaTentar.dd << "lo" << conferidaTentar.lo;

        const SenhaSegura porDe = SenhaSegura::de(bytes);
        const ConferenciaDaRegiao conferidaDe = conferirRegiaoDaSenha(porDe);
        const std::string rotuloDe = "de, tamanho " + std::to_string(tamanho) + ": " + conferidaDe.motivo;
        QVERIFY2(conferidaDe.motivo.empty(), rotuloDe.c_str());
        qInfo() << "de tamanho" << tamanho << "dd" << conferidaDe.dd << "lo" << conferidaDe.lo;
    }
}

// Gêmeo de tentarInicializaOSodium, por de(): em processo próprio, sodium_init() tem de devolver 1 depois de de().
void TesteSenhaSegura::deInicializaOSodium()
{
    static_cast<void>(SenhaSegura::de(QByteArrayLiteral("segredo")));
    QCOMPARE(sodium_init(), 1);
}

QTEST_APPLESS_MAIN(TesteSenhaSegura)
#include "tst_senha_segura.moc"

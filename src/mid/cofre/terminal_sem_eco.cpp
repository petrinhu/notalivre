// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura do terminal sem eco (NL-2.9, itens 2 e 3; NL-2.27, DA-2.68). Ver terminal_sem_eco.h. O buffer de leitura é zerado logo
// depois do uso. Dentro do Flatpak a CLI pode estar em grupo de segundo plano no terminal: nesse caso a leitura sai antes de qualquer
// tcsetattr, que pararia o processo com SIGTTOU.
#include <nl/mid/cofre/senha_do_terminal.h>
#include <nl/mid/cofre/terminal_sem_eco.h>

#include <openssl/crypto.h>

#include <QByteArrayView>

#include <array>
#include <cerrno>
#include <cstddef>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

namespace nl::mid::cofre {
namespace {

constexpr std::size_t kTetoDaLinha = 512;  // o mesmo teto para senha e confirmação; linha maior é recusada

using Buffer = std::array<char, kTetoDaLinha>;

// Resultado interno da leitura de uma linha do terminal: sem terminal, em segundo plano, falha, ou a linha lida.
enum class Leitura { Ok, SemTerminal, EmSegundoPlano, Falha };

// Restaura o eco na saída do escopo, mesmo que a leitura falhe.
class EcoRestaurado
{
public:
    EcoRestaurado(int fd, const termios& original) : fd_(fd), original_(original) {}
    ~EcoRestaurado() { tcsetattr(fd_, TCSANOW, &original_); }
    EcoRestaurado(const EcoRestaurado&) = delete;
    EcoRestaurado& operator=(const EcoRestaurado&) = delete;

private:
    int fd_;
    termios original_;
};

// Grava tudo ou falha. write() pode gravar menos que o pedido e pode ser interrompido (EINTR); o retorno não pode ser ignorado
// (com _FORTIFY_SOURCE, a glibc marca write como de retorno obrigatório, e o build do Flatpak trata o aviso como erro).
bool escreverTudo(int fd, const char* dados, std::size_t tamanho)
{
    while (tamanho > 0) {
        const ssize_t gravados = ::write(fd, dados, tamanho);
        if (gravados < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        dados += gravados;
        tamanho -= static_cast<std::size_t>(gravados);
    }
    return true;
}

// Confere que o processo está no primeiro plano do terminal (NL-2.27, DA-2.68). tcgetpgrp é permitido em segundo plano; tcgetattr e
// tcsetattr não são, e parariam o processo com SIGTTOU. Não lê nem altera nada no terminal.
Leitura conferirPrimeiroPlano(int fd)
{
    const pid_t primeiroPlano = tcgetpgrp(fd);
    if (primeiroPlano < 0) {
        return Leitura::Falha;
    }
    if (primeiroPlano != getpgrp()) {
        return Leitura::EmSegundoPlano;
    }
    return Leitura::Ok;
}

// Lê uma linha do /dev/tty, sem eco, para o buffer. A conferência do primeiro plano vem antes de qualquer tcsetattr.
Leitura lerLinhaNoBuffer(const QString& prompt, Buffer& buffer, std::size_t& tamanho)
{
    const int fd = ::open("/dev/tty", O_RDWR | O_NOCTTY);
    if (fd < 0) {
        return Leitura::SemTerminal;
    }
    if (const Leitura primeiroPlano = conferirPrimeiroPlano(fd); primeiroPlano != Leitura::Ok) {
        ::close(fd);
        return primeiroPlano;
    }
    termios original{};
    if (tcgetattr(fd, &original) != 0) {
        ::close(fd);
        return Leitura::Falha;
    }
    termios semEco = original;
    semEco.c_lflag &= ~ECHO;
    if (tcsetattr(fd, TCSANOW, &semEco) != 0) {
        ::close(fd);
        return Leitura::Falha;
    }
    bool estourou = false;
    bool gravou = true;
    {
        EcoRestaurado restaura{fd, original};
        const QByteArray texto = prompt.toLocal8Bit();
        gravou = escreverTudo(fd, texto.constData(), static_cast<std::size_t>(texto.size()));
        char caractere = 0;
        // Sem prompt gravado o terminal não está em uso correto: a leitura não começa e a chamada recusa.
        while (gravou && ::read(fd, &caractere, 1) == 1 && caractere != '\n') {
            if (caractere == '\r') {
                continue;
            }
            if (tamanho >= buffer.size()) {
                estourou = true;  // o resto da linha é lido e descartado; a chamada recusa
                continue;
            }
            buffer[tamanho++] = caractere;
        }
    }
    const char quebra = '\n';
    const bool quebrou = gravou && escreverTudo(fd, &quebra, 1);
    ::close(fd);
    return (quebrou && !estourou) ? Leitura::Ok : Leitura::Falha;
}

} // namespace

std::expected<void, ErroDoTerminal> conferirTerminalDaSenha()
{
    const int fd = ::open("/dev/tty", O_RDWR | O_NOCTTY);
    if (fd < 0) {
        return std::unexpected(ErroDoTerminal::SemTerminal);
    }
    const Leitura estado = conferirPrimeiroPlano(fd);
    ::close(fd);
    switch (estado) {
    case Leitura::Ok:
        return {};
    case Leitura::EmSegundoPlano:
        return std::unexpected(ErroDoTerminal::EmSegundoPlano);
    case Leitura::SemTerminal:
    case Leitura::Falha:
        break;
    }
    return std::unexpected(ErroDoTerminal::FalhaDeLeitura);
}

std::expected<nl::foundation::pkcs12::SenhaSegura, ErroDoTerminal> lerSenhaDoTerminal(const QString& prompt)
{
    Buffer buffer{};
    std::size_t tamanho = 0;
    const Leitura leitura = lerLinhaNoBuffer(prompt, buffer, tamanho);
    if (leitura != Leitura::Ok) {
        OPENSSL_cleanse(buffer.data(), buffer.size());
        switch (leitura) {
        case Leitura::EmSegundoPlano:
            return std::unexpected(ErroDoTerminal::EmSegundoPlano);
        case Leitura::SemTerminal:
            return std::unexpected(ErroDoTerminal::SemTerminal);
        case Leitura::Ok:
        case Leitura::Falha:
            break;
        }
        return std::unexpected(ErroDoTerminal::FalhaDeLeitura);
    }
    // NL-2.27, I3 (decisão do CISO de 01/11/2026): senha vazia no terminal é recusada aqui, como no descritor; não segue ao A1.
    if (tamanho == 0) {
        OPENSSL_cleanse(buffer.data(), buffer.size());
        return std::unexpected(ErroDoTerminal::SenhaVazia);
    }
    auto senha = nl::foundation::pkcs12::SenhaSegura::de(QByteArrayView(buffer.data(), static_cast<qsizetype>(tamanho)));
    OPENSSL_cleanse(buffer.data(), buffer.size());
    return senha;
}

std::optional<QString> lerConfirmacaoDoTerminal(const QString& prompt)
{
    Buffer buffer{};
    std::size_t tamanho = 0;
    if (lerLinhaNoBuffer(prompt, buffer, tamanho) != Leitura::Ok) {
        OPENSSL_cleanse(buffer.data(), buffer.size());
        return std::nullopt;
    }
    QString texto = QString::fromUtf8(QByteArrayView(buffer.data(), static_cast<qsizetype>(tamanho)));
    OPENSSL_cleanse(buffer.data(), buffer.size());
    return texto;
}

} // namespace nl::mid::cofre

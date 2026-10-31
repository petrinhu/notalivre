// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura do terminal sem eco (NL-2.9, itens 2 e 3). Ver terminal_sem_eco.h. O buffer de leitura é zerado logo depois do uso.
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

// Lê uma linha do /dev/tty, sem eco, para o buffer. Falso se não há terminal, se a leitura falha ou se a linha passa do teto.
bool lerLinhaNoBuffer(const QString& prompt, Buffer& buffer, std::size_t& tamanho)
{
    const int fd = ::open("/dev/tty", O_RDWR | O_NOCTTY);
    if (fd < 0) {
        return false;
    }
    termios original{};
    if (tcgetattr(fd, &original) != 0) {
        ::close(fd);
        return false;
    }
    termios semEco = original;
    semEco.c_lflag &= ~ECHO;
    if (tcsetattr(fd, TCSANOW, &semEco) != 0) {
        ::close(fd);
        return false;
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
    return quebrou && !estourou;
}

} // namespace

std::optional<nl::foundation::pkcs12::SenhaSegura> lerSenhaDoTerminal(const QString& prompt)
{
    Buffer buffer{};
    std::size_t tamanho = 0;
    if (!lerLinhaNoBuffer(prompt, buffer, tamanho)) {
        OPENSSL_cleanse(buffer.data(), buffer.size());
        return std::nullopt;
    }
    auto senha = nl::foundation::pkcs12::SenhaSegura::de(QByteArrayView(buffer.data(), static_cast<qsizetype>(tamanho)));
    OPENSSL_cleanse(buffer.data(), buffer.size());
    return senha;
}

std::optional<QString> lerConfirmacaoDoTerminal(const QString& prompt)
{
    Buffer buffer{};
    std::size_t tamanho = 0;
    if (!lerLinhaNoBuffer(prompt, buffer, tamanho)) {
        OPENSSL_cleanse(buffer.data(), buffer.size());
        return std::nullopt;
    }
    QString texto = QString::fromUtf8(QByteArrayView(buffer.data(), static_cast<qsizetype>(tamanho)));
    OPENSSL_cleanse(buffer.data(), buffer.size());
    return texto;
}

} // namespace nl::mid::cofre

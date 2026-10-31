// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura do terminal sem eco (NL-2.9, itens 2 e 3). Ver terminal_sem_eco.h. O buffer de leitura é zerado logo depois do uso.
#include <nl/mid/cofre/terminal_sem_eco.h>

#include <openssl/crypto.h>

#include <QByteArrayView>

#include <array>
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
    {
        EcoRestaurado restaura{fd, original};
        const QByteArray texto = prompt.toLocal8Bit();
        static_cast<void>(::write(fd, texto.constData(), static_cast<std::size_t>(texto.size())));
        char caractere = 0;
        while (::read(fd, &caractere, 1) == 1 && caractere != '\n') {
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
    static_cast<void>(::write(fd, &quebra, 1));
    ::close(fd);
    return !estourou;
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

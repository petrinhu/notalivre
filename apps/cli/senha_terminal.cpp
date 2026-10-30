// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Leitura de senha do terminal sem eco (NL-2.9). Ver senha_terminal.h. WIP: sem verificação.
#include "senha_terminal.h"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace nl::cli {
namespace {

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

} // namespace

std::optional<QByteArray> lerSenhaSemEco(const QString& prompt)
{
    const int fd = ::open("/dev/tty", O_RDWR | O_NOCTTY);
    if (fd < 0) {
        return std::nullopt;
    }
    termios original{};
    if (tcgetattr(fd, &original) != 0) {
        ::close(fd);
        return std::nullopt;
    }
    termios semEco = original;
    semEco.c_lflag &= ~ECHO;
    if (tcsetattr(fd, TCSANOW, &semEco) != 0) {
        ::close(fd);
        return std::nullopt;
    }
    QByteArray senha;
    {
        EcoRestaurado restaura{fd, original};
        const QByteArray texto = prompt.toLocal8Bit();
        ::write(fd, texto.constData(), static_cast<std::size_t>(texto.size()));
        char caractere = 0;
        while (::read(fd, &caractere, 1) == 1 && caractere != '\n') {
            if (caractere != '\r') {
                senha.append(caractere);
            }
        }
    }
    const char quebra = '\n';
    ::write(fd, &quebra, 1);
    ::close(fd);
    return senha;
}

} // namespace nl::cli

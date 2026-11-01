// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Posse do descritor da senha na entrada da CLI (NL-2.27b, DA-2.72). Ver tomada_descritor.h. A tomada não lê bytes: confere o número
// pedido por fstat e isatty, guarda a identidade, duplica para um número alto com FD_CLOEXEC e fecha o número pedido.
#include <nl/mid/cofre/tomada_descritor.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <climits>
#include <cstdlib>
#include <string>

namespace nl::mid::cofre {
namespace {

// Número alto da posse: fica longe dos números que o app abre depois (arquivos, sockets, bibliotecas do Qt).
constexpr int kMenorNumeroDaPosse = 100;

// Caminho do arquivo que ocupa o número, lido do /proc (nunca o conteúdo). Vazio se não há link.
std::string caminhoDoOcupante(int numero)
{
    const std::string enlace = "/proc/self/fd/" + std::to_string(numero);
    char alvo[PATH_MAX];
    const ssize_t tamanho = readlink(enlace.c_str(), alvo, sizeof alvo - 1);
    if (tamanho <= 0) {
        return {};
    }
    return std::string(alvo, static_cast<std::size_t>(tamanho));
}

} // namespace

std::expected<TomadaDeDescritor, RecusaDaTomada> tomarDescritorDeSenha(int numeroPedido)
{
    // 0, 1 e 2 e número não aberto não são do app: nada é fechado (DA-2.16).
    if (numeroPedido <= 2) {
        return std::unexpected(RecusaDaTomada{ErroDaTomada::DescritorInvalido, {}});
    }
    struct stat estado{};
    if (fstat(numeroPedido, &estado) != 0) {
        return std::unexpected(RecusaDaTomada{ErroDaTomada::DescritorInvalido, {}});
    }
    if (isatty(numeroPedido) == 1) {
        ::close(numeroPedido);
        return std::unexpected(RecusaDaTomada{ErroDaTomada::DescritorTerminal, {}});
    }
    if (S_ISREG(estado.st_mode) && (estado.st_mode & 077) != 0) {
        RecusaDaTomada recusa{ErroDaTomada::LegivelPorOutros, caminhoDoOcupante(numeroPedido)};
        ::close(numeroPedido);
        return std::unexpected(recusa);
    }
    const int posse = fcntl(numeroPedido, F_DUPFD_CLOEXEC, kMenorNumeroDaPosse);
    if (posse < 0) {
        ::close(numeroPedido);
        return std::unexpected(RecusaDaTomada{ErroDaTomada::FalhaDeTomada, {}});
    }
    ::close(numeroPedido);
    return TomadaDeDescritor{posse, estado.st_dev, estado.st_ino};
}

std::expected<void, RecusaDaTomada> conferirTomadaDeDescritor(const TomadaDeDescritor& tomada)
{
    struct stat estado{};
    if (fstat(tomada.fd, &estado) != 0 || estado.st_dev != tomada.dispositivo || estado.st_ino != tomada.inodo) {
        return std::unexpected(RecusaDaTomada{ErroDaTomada::DescritorTrocado, {}});
    }
    return {};
}

} // namespace nl::mid::cofre

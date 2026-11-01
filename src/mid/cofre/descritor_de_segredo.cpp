// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Canal de segredo por descritor (NL-2.27, DA-2.16, Emenda 1 do CISO). Ver descritor_de_segredo.h. A conferência do descritor (fd
// 0 a 2, aberto, terminal, permissão de grupo ou outros) acontece antes de qualquer leitura. Depois da leitura, o buffer é zerado e
// o descritor fechado.
#include <nl/mid/cofre/descritor_de_segredo.h>

#include <openssl/crypto.h>

#include <QByteArrayView>

#include <array>
#include <cerrno>
#include <cstddef>
#include <optional>

#include <sys/stat.h>
#include <unistd.h>

namespace nl::mid::cofre {
namespace {

constexpr std::size_t kTetoDoSegredo = 512;  // o mesmo teto do terminal; um byte a mais é lido só para detectar o estouro

using Buffer = std::array<char, kTetoDoSegredo + 1>;

// Conferência do descritor, sem ler nenhum byte: 0 a 2 e negativos são recusados; descritor que não está aberto (fstat falha) é
// inválido; terminal é recusado; arquivo comum com permissão para grupo ou outros é recusado (mode & 077, falha fechada).
std::optional<ErroDoSegredo> conferirDescritor(int fd)
{
    if (fd <= 2) {
        return ErroDoSegredo::DescritorInvalido;
    }
    struct stat estado{};
    if (fstat(fd, &estado) != 0) {
        return ErroDoSegredo::DescritorInvalido;
    }
    if (isatty(fd) == 1) {
        return ErroDoSegredo::DescritorTerminal;
    }
    if (S_ISREG(estado.st_mode) && (estado.st_mode & 077) != 0) {
        return ErroDoSegredo::LegivelPorOutros;
    }
    return std::nullopt;
}

// Lê do descritor até o fim do arquivo ou até passar do teto. Devolve os bytes lidos (no máximo o teto mais um).
std::expected<std::size_t, ErroDoSegredo> lerAteOTeto(int fd, Buffer& buffer)
{
    std::size_t total = 0;
    while (total < buffer.size()) {
        const ssize_t lidos = ::read(fd, buffer.data() + total, buffer.size() - total);
        if (lidos < 0) {
            if (errno == EINTR) {
                continue;
            }
            return std::unexpected(ErroDoSegredo::FalhaDeLeitura);
        }
        if (lidos == 0) {
            break;
        }
        total += static_cast<std::size_t>(lidos);
    }
    return total;
}

} // namespace

std::expected<nl::foundation::pkcs12::SenhaSegura, ErroDoSegredo> lerSenhaDoDescritor(int fd)
{
    if (auto recusa = conferirDescritor(fd)) {
        return std::unexpected(*recusa);
    }
    Buffer buffer{};
    auto lidos = lerAteOTeto(fd, buffer);
    ::close(fd);
    if (!lidos) {
        OPENSSL_cleanse(buffer.data(), buffer.size());
        return std::unexpected(lidos.error());
    }
    std::size_t tamanho = *lidos;
    if (tamanho > kTetoDoSegredo) {
        OPENSSL_cleanse(buffer.data(), buffer.size());
        return std::unexpected(ErroDoSegredo::AcimaDoTeto);
    }
    // Uma só quebra final sai (Emenda 1, item 4). Espaço, tabulação e retorno de carro nunca são aparados.
    if (tamanho > 0 && buffer[tamanho - 1] == '\n') {
        --tamanho;
    }
    if (tamanho == 0) {
        OPENSSL_cleanse(buffer.data(), buffer.size());
        return std::unexpected(ErroDoSegredo::SenhaVazia);
    }
    auto senha = nl::foundation::pkcs12::SenhaSegura::de(QByteArrayView(buffer.data(), static_cast<qsizetype>(tamanho)));
    OPENSSL_cleanse(buffer.data(), buffer.size());
    return senha;
}

// STUB do passo vermelho (NL-2.27): devolve o comportamento antigo, sem tocar no buffer, para o teste falhar por asserção.
std::expected<nl::foundation::pkcs12::SenhaSegura, ErroDoSegredo> lerSenhaDoDescritorEm(int fd,
                                                                                         std::array<char, kTamanhoBufferDoSegredo>& buffer)
{
    static_cast<void>(buffer);
    static_cast<void>(fd);
    return std::unexpected(ErroDoSegredo::DescritorInvalido);
}

// STUB do passo vermelho (NL-2.27): ainda não confere nada.
std::expected<void, ErroDoSegredo> conferirDescritorDeSenha(int fd)
{
    static_cast<void>(fd);
    return std::unexpected(ErroDoSegredo::DescritorInvalido);
}

} // namespace nl::mid::cofre

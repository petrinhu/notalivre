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

constexpr std::size_t kTetoDoSegredo = 512;  // o mesmo teto do terminal; a quebra final não conta e os bytes extras são estouro

using Buffer = std::array<char, kTamanhoBufferDoSegredo>;

// Zera o buffer do chamador na saída do escopo, em qualquer caminho (sucesso, recusa ou falha de leitura).
class BufferZerado
{
public:
    explicit BufferZerado(Buffer& buffer) : buffer_(buffer) {}
    ~BufferZerado() { OPENSSL_cleanse(buffer_.data(), buffer_.size()); }
    BufferZerado(const BufferZerado&) = delete;
    BufferZerado& operator=(const BufferZerado&) = delete;

private:
    Buffer& buffer_;
};

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

// Lê do descritor até o fim do arquivo ou até encher o buffer (teto, quebra final e um byte de estouro). Devolve os bytes lidos.
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

std::expected<nl::foundation::pkcs12::SenhaSegura, ErroDoSegredo> lerSenhaDoDescritorEm(int fd, std::array<char, kTamanhoBufferDoSegredo>& buffer)
{
    BufferZerado zerado{buffer};
    if (auto recusa = conferirDescritor(fd)) {
        return std::unexpected(*recusa);
    }
    auto lidos = lerAteOTeto(fd, buffer);
    ::close(fd);
    if (!lidos) {
        return std::unexpected(lidos.error());
    }
    std::size_t tamanho = *lidos;
    // Uma só quebra final sai (Emenda 1, item 4), antes de qualquer conta do teto. Espaço, tabulação e retorno de carro nunca são
    // aparados. Com 512 bytes mais a quebra, o conteúdo tem 512 e passa; com um byte a mais, estoura.
    if (tamanho > 0 && buffer[tamanho - 1] == '\n') {
        --tamanho;
    }
    if (tamanho > kTetoDoSegredo) {
        return std::unexpected(ErroDoSegredo::AcimaDoTeto);
    }
    if (tamanho == 0) {
        return std::unexpected(ErroDoSegredo::SenhaVazia);
    }
    return nl::foundation::pkcs12::SenhaSegura::de(QByteArrayView(buffer.data(), static_cast<qsizetype>(tamanho)));
}

std::expected<nl::foundation::pkcs12::SenhaSegura, ErroDoSegredo> lerSenhaDoDescritor(int fd)
{
    Buffer buffer{};
    return lerSenhaDoDescritorEm(fd, buffer);
}

std::expected<void, ErroDoSegredo> conferirDescritorDeSenha(int fd)
{
    if (auto recusa = conferirDescritor(fd)) {
        return std::unexpected(*recusa);
    }
    return {};
}

} // namespace nl::mid::cofre

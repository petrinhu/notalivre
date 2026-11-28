// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Evento sefin.resposta_sem_codigo (NL-3.L, passo 15). Ver registro_resposta_sem_codigo.h. O corpo da resposta nunca entra aqui:
// a função recebe só o tamanho dele.
#include <nl/mid/sefin/cliente/registro_resposta_sem_codigo.h>

#include <QByteArray>

#include <vector>

namespace nl::mid::sefin::cliente {

namespace fl = nl::foundation::log;

std::string tipoMidiaParaLog(QByteArrayView tipoConteudo)
{
    const qsizetype fim = tipoConteudo.indexOf(';');
    const QByteArray midia = (fim < 0 ? tipoConteudo : tipoConteudo.first(fim)).trimmed().toByteArray().toLower();
    if (midia.isEmpty()) {
        return "ausente";
    }
    if (midia == "application/json" || midia.endsWith("+json")) {
        return "json";
    }
    if (midia == "text/html" || midia == "application/xhtml+xml") {
        return "html";
    }
    if (midia == "application/xml" || midia == "text/xml" || midia.endsWith("+xml")) {
        return "xml";
    }
    if (midia == "text/plain") {
        return "texto";
    }
    return "outro";
}

void registrarRespostaSemCodigo(fl::Log& registro, int status, QByteArrayView tipoConteudo, qsizetype bytesResposta)
{
    const std::vector<fl::Campo> campos{
        fl::Campo{"status", std::to_string(status)},
        fl::Campo{"tipoMidia", tipoMidiaParaLog(tipoConteudo)},
        fl::Campo{"bytesResposta", std::to_string(bytesResposta < 0 ? 0 : bytesResposta)},
    };
    (void)registro.registrar(fl::Nivel::Erro, "sefin.resposta_sem_codigo", campos);
}

} // namespace nl::mid::sefin::cliente

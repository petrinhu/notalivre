// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Manipulador de mensagens do Qt ligado ao Log (NL-2.9, ADR-0005 a). Ver manipulador_qt.h. Desligar o manipulador instala o
// que descarta a mensagem, nunca o padrão do Qt (NL-2.9, item 4): o padrão escreveria a mensagem crua na saída de erro.
#include <nl/mid/composicao/manipulador_qt.h>

#include <nl/foundation/log/log.h>

#include <QByteArray>
#include <QMessageLogContext>
#include <QString>

namespace nl::mid::composicao {
namespace {

nl::foundation::log::Log* registroAtivo = nullptr;  // exigido pela assinatura de qInstallMessageHandler (ver manipulador_qt.h)

nl::foundation::log::Nivel nivelDe(QtMsgType tipo)
{
    switch (tipo) {
    case QtDebugMsg:
    case QtInfoMsg:
        return nl::foundation::log::Nivel::Info;
    case QtWarningMsg:
        return nl::foundation::log::Nivel::Aviso;
    case QtCriticalMsg:
    case QtFatalMsg:
        return nl::foundation::log::Nivel::Erro;
    }
    return nl::foundation::log::Nivel::Erro;
}

void manipular(QtMsgType tipo, const QMessageLogContext& contexto, const QString& mensagem)
{
    static_cast<void>(contexto);
    if (registroAtivo == nullptr) {
        return;  // falha fechada: sem Log, nada sai pelo Qt
    }
    // O campo "texto" não é declarado na allowlist: o Log descarta a mensagem inteira e conta o descarte.
    static_cast<void>(registroAtivo->registrar(nivelDe(tipo), "qt.mensagem", {nl::foundation::log::Campo{"texto", mensagem.toStdString()}}));
}

// Manipulador de descarte: é o que fica instalado depois de desligar. Não escreve nada e não toca o Log, que pode já ter sido destruído.
void descartar(QtMsgType tipo, const QMessageLogContext& contexto, const QString& mensagem)
{
    static_cast<void>(tipo);
    static_cast<void>(contexto);
    static_cast<void>(mensagem);
}

} // namespace

void instalarManipuladorQt(nl::foundation::log::Log* registro)
{
    registroAtivo = registro;
    qInstallMessageHandler(registro != nullptr ? manipular : descartar);
}

} // namespace nl::mid::composicao

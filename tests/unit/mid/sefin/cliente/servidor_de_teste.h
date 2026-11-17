// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Teste do ClienteSefin: o servidor falso da Sefin em processo próprio, com as falhas do roteiro.
// Corpo movido byte a byte de apoio_cliente_sefin.h (NL-3.C, S5c, DA-3C.25). Namespace anônimo: uma cópia por executável.
#ifndef NL_TESTE_SERVIDOR_DE_TESTE_H
#define NL_TESTE_SERVIDOR_DE_TESTE_H

#include <QProcess>
#include <QString>
#include <QStringList>

namespace {

// Servidor falso em processo próprio, com oráculo (--com-oraculo), e as falhas do roteiro. Fecha o stdin ao sair.
class ServidorDeTeste {
public:
    explicit ServidorDeTeste(const QStringList& falhas, const QString& tlsMaximo = {})
    {
        QStringList argumentos{QStringLiteral("-I"), qEnvironmentVariable("NL_LANCADOR"),
                               qEnvironmentVariable("NL_CERTIFICADOS_DIR"), QStringLiteral("--com-oraculo")};
        for (const QString& falha : falhas) {
            argumentos << QStringLiteral("--falha") << falha.split(QLatin1Char(' '));
        }
        if (!tlsMaximo.isEmpty()) {
            argumentos << QStringLiteral("--tls-maximo") << tlsMaximo;  // versão máxima de TLS do cenário (rodada 4)
        }
        processo_.start(qEnvironmentVariable("NL_PYTHON3"), argumentos);
        if (processo_.waitForReadyRead(20000)) {
            const QString linha = QString::fromUtf8(processo_.readLine()).trimmed();
            if (linha.startsWith(QStringLiteral("PORTA "))) {
                porta_ = linha.mid(6).toInt();
            }
        }
        if (porta_ == 0) {
            diagnostico_ = QStringLiteral("servidor falso não subiu; stderr: ") +
                           QString::fromUtf8(processo_.readAllStandardError());
        }
    }
    ~ServidorDeTeste()
    {
        processo_.closeWriteChannel();
        processo_.waitForFinished(5000);
    }
    int porta() const { return porta_; }
    const QString& diagnostico() const { return diagnostico_; }

private:
    QProcess processo_;
    int porta_ = 0;
    QString diagnostico_;
};

} // namespace

#endif // NL_TESTE_SERVIDOR_DE_TESTE_H

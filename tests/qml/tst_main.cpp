// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Ponto de entrada dos testes QML (NL-1.8). Roda offscreen (L-50): QT_QPA_PLATFORM vem do CTest.
// Carrega as mesmas fontes da GUI a partir da árvore de origem, e expõe ao QML o diretório de capturas
// (variável NL_CAPTURAS_DIR, definida pelo CTest; vazia faz a captura ser pulada).

#include <QByteArray>
#include <QFontDatabase>
#include <QObject>
#include <QQmlContext>
#include <QQmlEngine>
#include <QString>

#include <QtQuickTest/quicktest.h>

#ifndef NL_FONTES_DIR
#error "NL_FONTES_DIR deve vir do CMake (diretório de origem das fontes da GUI)"
#endif

class PreparoQmlTeste : public QObject
{
    Q_OBJECT

public slots:
    void applicationAvailable()
    {
        const char *arquivos[] = {
            "BricolageGrotesque-VF.ttf",
            "AtkinsonHyperlegible-Regular.ttf",
            "AtkinsonHyperlegible-Bold.ttf",
        };
        for (const char *nome : arquivos) {
            const QString caminho = QStringLiteral(NL_FONTES_DIR) + QLatin1Char('/') + QLatin1String(nome);
            QFontDatabase::addApplicationFont(caminho);
        }
    }

    void qmlEngineAvailable(QQmlEngine *motor)
    {
        const QByteArray capturas = qgetenv("NL_CAPTURAS_DIR");
        motor->rootContext()->setContextProperty(QStringLiteral("capturasDir"),
                                                 QString::fromLocal8Bit(capturas));
    }
};

QUICK_TEST_MAIN_WITH_SETUP(tst_qml_gui, PreparoQmlTeste)

#include "tst_main.moc"

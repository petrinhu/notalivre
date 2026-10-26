// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Ponto de entrada da GUI (NL-1.8): carrega as fontes do recurso e abre a janela da casca em QML.
// Falha de fonte encerra o app (fail-closed): sem a fonte de marca o visual não é o aprovado.

#include <QCoreApplication>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QString>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    const char *fontes[] = {
        ":/fontes/BricolageGrotesque-VF.ttf",
        ":/fontes/AtkinsonHyperlegible-Regular.ttf",
        ":/fontes/AtkinsonHyperlegible-Bold.ttf",
    };
    for (const char *caminho : fontes) {
        if (QFontDatabase::addApplicationFont(QString::fromLatin1(caminho)) < 0)
            return 1;
    }

    QQmlApplicationEngine motor;
    motor.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/qml"));
    motor.loadFromModule(QStringLiteral("NotaLivre.Gui"), QStringLiteral("Janela"));
    if (motor.rootObjects().isEmpty())
        return 1;

    return app.exec();
}

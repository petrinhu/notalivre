// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Smoke headless da NL-0.5: carrega QML num motor, sem janela visível, e imprime a versão do Qt em execução.
// Protótipo descartável de medição, declarado fora do TDD (L-35). Roda só com QT_QPA_PLATFORM=offscreen (L-50).
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtGlobal>
#include <cstdio>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine motor;
    motor.loadData(QByteArrayLiteral("import QtQuick\nItem {}\n"));
    std::printf("qt_em_execucao=%s\n", qVersion());
    std::printf("raizes_qml=%lld\n", static_cast<long long>(motor.rootObjects().size()));
    return motor.rootObjects().isEmpty() ? 1 : 0;
}

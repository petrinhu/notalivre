// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Átomo próprio (NL-3.C; S5, DA-3C.22): o corpo foi movido byte a byte de tst_cliente_sefin.cpp. Um uso só (4.6b).
#ifndef NL_TESTE_REPASSADOR_TCP_H
#define NL_TESTE_REPASSADOR_TCP_H

#include <QHostAddress>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

#include <memory>
#include <vector>

namespace nl::teste {

// Repassador TCP de teste (4.6b): a conexão 1 (POST) e a 2 (GET /dps) vão ao servidor falso; da 3 em diante, cada conexão
// fecha na chegada, sem repassar byte. Conta as conexões e as repassadas. Tudo por sinal, no laço do próprio cliente.
class RepassadorTcp {
public:
    RepassadorTcp(int portaDestino, int repassarAte) : destino_(portaDestino), repassarAte_(repassarAte)
    {
        QObject::connect(&servidor_, &QTcpServer::newConnection, [this] { aceitar(); });
    }
    bool escutar() { return servidor_.listen(QHostAddress::LocalHost); }
    int porta() const { return servidor_.serverPort(); }
    int conexoes() const { return conexoes_; }
    int repassadas() const { return repassadas_; }

private:
    void aceitar()
    {
        while (QTcpSocket* recebida = servidor_.nextPendingConnection()) {
            sockets_.emplace_back(recebida);
            ++conexoes_;
            if (conexoes_ > repassarAte_) {
                recebida->disconnectFromHost();
                continue;
            }
            ++repassadas_;
            encaminhar(recebida);
        }
    }

    void encaminhar(QTcpSocket* cliente)
    {
        sockets_.push_back(std::make_unique<QTcpSocket>());
        QTcpSocket* saida = sockets_.back().get();
        auto pendente = std::make_shared<QByteArray>();
        QObject::connect(cliente, &QTcpSocket::readyRead, [cliente, saida, pendente] {
            const QByteArray dados = cliente->readAll();
            if (saida->state() == QAbstractSocket::ConnectedState) {
                saida->write(dados);
            } else {
                pendente->append(dados);
            }
        });
        QObject::connect(saida, &QTcpSocket::connected, [saida, pendente] {
            saida->write(*pendente);
            pendente->clear();
        });
        QObject::connect(saida, &QTcpSocket::readyRead, [cliente, saida] { cliente->write(saida->readAll()); });
        QObject::connect(cliente, &QTcpSocket::disconnected, [saida] { saida->disconnectFromHost(); });
        QObject::connect(saida, &QTcpSocket::disconnected, [cliente] { cliente->disconnectFromHost(); });
        saida->connectToHost(QHostAddress::LocalHost, destino_);
    }

    int destino_;
    int repassarAte_;
    int conexoes_ = 0;
    int repassadas_ = 0;
    QTcpServer servidor_;
    std::vector<std::unique_ptr<QTcpSocket>> sockets_;
};

} // namespace nl::teste

#endif // NL_TESTE_REPASSADOR_TCP_H

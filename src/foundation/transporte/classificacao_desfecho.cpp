// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Átomo do desfecho do Qt (NL-3.C, plano seção 1.7). Ver classificacao_desfecho.h. Comportamento movido de
// transporte_https.cpp sem alterar decisão alguma (passo 3.2 da ordem de serviço).
#include <nl/foundation/transporte/classificacao_desfecho.h>

#include <nl/foundation/transporte/classificacao_erro.h>

namespace nl::foundation::transporte {

ErroTransporte erro(TipoErro tipo, CausaErro causa, QString mensagem)
{
    ErroTransporte resultado;
    resultado.tipo = tipo;
    resultado.causa = causa;
    resultado.mensagem = std::move(mensagem);
    return resultado;
}

namespace {

// Descrição fixa por causa: nenhum texto vindo de fora entra na mensagem.
QString descricaoDaCausa(CausaErro causa)
{
    switch (causa) {
    case CausaErro::Timeout:
        return QStringLiteral("tempo limite esgotado");
    case CausaErro::Conexao:
        return QStringLiteral("conexão indisponível ou interrompida");
    case CausaErro::Tls:
        return QStringLiteral("handshake TLS recusado");
    case CausaErro::Protocolo:
        return QStringLiteral("resposta de rede inválida");
    case CausaErro::Configuracao:
        return QStringLiteral("configuração incompleta");
    }
    return QStringLiteral("erro de transporte");
}

QString mensagemDeRede(CausaErro causa, QNetworkReply::NetworkError codigo, bool handshake, int sslErros)
{
    return QStringLiteral("%1 (codigo Qt %2, handshake %3, erros SSL %4)")
        .arg(descricaoDaCausa(causa))
        .arg(static_cast<int>(codigo))
        .arg(handshake ? QStringLiteral("sim") : QStringLiteral("não"))
        .arg(sslErros);
}

// Erro de rede sem status: a classificação é do átomo; aqui só se monta a mensagem fixa.
ErroTransporte erroDeRede(QNetworkReply::NetworkError codigo, bool handshake, int sslErros)
{
    const Classificacao classe = classificarErroDeRede(codigo);
    ErroTransporte erroRede = erro(classe.tipo, classe.causa, mensagemDeRede(classe.causa, codigo, handshake, sslErros));
    erroRede.codigoQt = static_cast<int>(codigo);  // NL-2.10e (E4): para o log de falha de transporte
    erroRede.handshake = handshake;
    return erroRede;
}

// NL-3.C (plano 7.1): todo ramo de erro leva o código do Qt e o estado do handshake do desfecho (requisito 3 do CISO).
ErroTransporte comDadosDoQt(ErroTransporte erroTransporte, const Desfecho& d)
{
    erroTransporte.codigoQt = static_cast<int>(d.codigo);
    erroTransporte.handshake = d.handshake;
    return erroTransporte;
}

// Transforma o que o Qt observou no resultado do transporte. Ordem: teto do corpo, tempo limite, status recebido,
// e por fim erro sem status.
std::expected<RespostaHttp, ErroTransporte> classificarDesfechoSemDados(const Desfecho& d)
{
    if (d.excedeuTeto) {
        return std::unexpected(comDadosDoQt(erro(TipoErro::Definitivo, CausaErro::Protocolo,
                                    mensagemDeRede(CausaErro::Protocolo, d.codigo, d.handshake, d.sslErros)), d));
    }
    if (d.expirou) {
        return std::unexpected(comDadosDoQt(erro(TipoErro::Transitorio, CausaErro::Timeout,
                                    mensagemDeRede(CausaErro::Timeout, d.codigo, d.handshake, d.sslErros)), d));
    }
    if (d.status > 0) {
        if (ehRedirecionamento(d.status) || !d.contentEncodingIdentidade) {
            return std::unexpected(comDadosDoQt(erro(TipoErro::Definitivo, CausaErro::Protocolo,
                                        mensagemDeRede(CausaErro::Protocolo, d.codigo, d.handshake, d.sslErros)), d));
        }
        if (ehCodigoDeRespostaCompleta(d.codigo)) {
            return RespostaHttp{d.status, d.corpo, d.tipoConteudo};
        }
        // Erro de rede (1 a 99) depois do status: corpo incompleto ou conexão cortada, nunca resposta.
        const int codigo = static_cast<int>(d.codigo);
        if (codigo >= 1 && codigo <= 99) {
            return std::unexpected(comDadosDoQt(erro(TipoErro::Transitorio, CausaErro::Conexao,
                                        mensagemDeRede(CausaErro::Conexao, d.codigo, d.handshake, d.sslErros)), d));
        }
        return std::unexpected(comDadosDoQt(erro(TipoErro::Definitivo, CausaErro::Protocolo,
                                    mensagemDeRede(CausaErro::Protocolo, d.codigo, d.handshake, d.sslErros)), d));
    }
    // Recusa do servidor depois do handshake: sinal encrypted, sem status, código UnknownNetworkError (DA-2.35).
    if (d.handshake && d.codigo == QNetworkReply::UnknownNetworkError) {
        return std::unexpected(comDadosDoQt(erro(TipoErro::Definitivo, CausaErro::Tls,
                                    QStringLiteral("servidor recusou depois do handshake, provável certificado do cliente")), d));
    }
    return std::unexpected(erroDeRede(d.codigo, d.handshake, d.sslErros));
}

// NL-2.10e (E4 complemento, DA-2.75): anexa ao erro os dados TLS do desfecho (códigos SSL, versão e protocolo). Só valores do conjunto
// fechado; nunca texto do Qt, certificado ou erro anexado.
std::expected<RespostaHttp, ErroTransporte> classificarDesfecho(const Desfecho& d)
{
    auto resultado = classificarDesfechoSemDados(d);
    if (!resultado) {
        resultado.error().codigosSsl = d.codigosSsl;
        resultado.error().versaoTls = d.versaoTls;
        resultado.error().protocoloNegociado = d.protocoloNegociado;
    }
    return resultado;
}

} // namespace

// Resultado do pedido, com os bytes escritos do corpo no erro: é o sinal que o cliente usa para decidir se consulta (DA-2.41).
std::expected<RespostaHttp, ErroTransporte> interpretarDesfecho(const Desfecho& d)
{
    auto resultado = classificarDesfecho(d);
    if (!resultado) {
        resultado.error().bytesEscritos = d.bytesEscritos;
    }
    return resultado;
}

} // namespace nl::foundation::transporte

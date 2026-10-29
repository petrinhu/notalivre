// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Átomo de classificação (NL-2.5). Ver classificacao_erro.h. Puro: sem rede, sem Qt além dos enums.
#include <nl/foundation/transporte/classificacao_erro.h>

namespace nl::foundation::transporte {

Classificacao classificarErroDeRede(QNetworkReply::NetworkError codigo)
{
    switch (codigo) {
    case QNetworkReply::TimeoutError:
        return Classificacao{TipoErro::Transitorio, CausaErro::Timeout};
    case QNetworkReply::SslHandshakeFailedError:
        return Classificacao{TipoErro::Definitivo, CausaErro::Tls};
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::NetworkSessionFailedError:
        return Classificacao{TipoErro::Transitorio, CausaErro::Conexao};
    default:
        return Classificacao{TipoErro::Definitivo, CausaErro::Protocolo};
    }
}

bool ehCodigoDeRespostaCompleta(QNetworkReply::NetworkError codigo)
{
    switch (codigo) {
    case QNetworkReply::NoError:
    case QNetworkReply::ContentAccessDenied:
    case QNetworkReply::ContentOperationNotPermittedError:
    case QNetworkReply::ContentNotFoundError:
    case QNetworkReply::AuthenticationRequiredError:
    case QNetworkReply::ContentReSendError:
    case QNetworkReply::ContentConflictError:
    case QNetworkReply::ContentGoneError:
    case QNetworkReply::UnknownContentError:
    case QNetworkReply::ProtocolInvalidOperationError:
    case QNetworkReply::InternalServerError:
    case QNetworkReply::OperationNotImplementedError:
    case QNetworkReply::ServiceUnavailableError:
    case QNetworkReply::UnknownServerError:
        return true;
    default:
        return false;
    }
}

bool ehRedirecionamento(int status)
{
    return status >= 300 && status <= 399;
}

} // namespace nl::foundation::transporte

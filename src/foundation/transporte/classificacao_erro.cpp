// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Átomo de classificação (NL-2.5). Ver classificacao_erro.h.
// Fase vermelha: o stub não classifica nada. A tabela de testes reprova por asserção em cada regra.
#include <nl/foundation/transporte/classificacao_erro.h>

namespace nl::foundation::transporte {

Classificacao classificarErroDeRede(QNetworkReply::NetworkError)
{
    return Classificacao{TipoErro::Definitivo, CausaErro::Protocolo};
}

bool ehCodigoDeRespostaCompleta(QNetworkReply::NetworkError)
{
    return false;
}

bool ehRedirecionamento(int)
{
    return false;
}

} // namespace nl::foundation::transporte

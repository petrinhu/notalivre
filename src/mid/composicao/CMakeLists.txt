# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Composição (NL-1.7): fachadas que a CLI pode chamar. A fachada da DPS lê o JSON, monta pelo Back, serializa e valida no XSD.
# Cada arquivo é um átomo (L-34); os leitores de grupo ficam em arquivos próprios e o header interno é leitores_dps.h.
target_sources(nl_mid PRIVATE
    confirmacao_inscricao.cpp
    campos_json.cpp
    dps_validar.cpp
    emissao_nota.cpp
    manipulador_qt.cpp
    guarda_manipulador_qt.cpp
    registro_cli.cpp
    codigo_saida.cpp
    resposta_crua.cpp
    politica_emissao.cpp
    emissao_cli.cpp
    ler_dados_dps.cpp
    ler_endereco.cpp
    ler_identificacao.cpp
    ler_ibscbs.cpp
    ler_inscricao.cpp
    ler_prestador.cpp
    ler_regime.cpp
    ler_servico.cpp
    ler_tomador.cpp
    ler_tottrib.cpp
    ler_tribmun.cpp
    ler_valor.cpp
    ler_valores.cpp)

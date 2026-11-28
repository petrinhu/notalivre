# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Composição (NL-1.7): fachadas que a CLI pode chamar. A fachada da DPS lê o JSON, monta pelo Back, serializa e valida no XSD.
# Cada arquivo é um átomo (L-34); os leitores de grupo ficam em arquivos próprios e o header interno é leitores_dps.h.
target_sources(nl_mid PRIVATE
    confirmacao_inscricao.cpp
    campos_json.cpp
    dps_validar.cpp
    regras_locais.cpp
    emissao_nota.cpp
    manipulador_qt.cpp
    guarda_manipulador_qt.cpp
    registro_cli.cpp
    codigo_saida.cpp
    catalogo_falha_envio.cpp
    classe_falha_envio.cpp
    resposta_crua.cpp
    politica_emissao.cpp
    emissao_cli.cpp
    ler_dados_dps.cpp
    ler_endereco.cpp
    ler_identificacao.cpp
    ler_ibscbs.cpp
    ler_indicador_destinatario.cpp
    ler_inscricao.cpp
    ler_prestador.cpp
    ler_regime.cpp
    ler_servico.cpp
    ler_tomador.cpp
    ler_tottrib.cpp
    ler_tribmun.cpp
    ler_valor.cpp
    ler_valores.cpp)

# Pin do manifesto do XSD (NL-2.10b, BAIXA-4): só dps_validar.cpp lê a definição; o hash é o do SHA256SUMS de third_party.
target_compile_definitions(nl_mid PRIVATE "NL_XSD_MANIFESTO_SHA256=\"${NL_XSD_PRODREST_MANIFESTO_SHA256}\"")
# Pin do manifesto das tabelas (NL-3.M, DA-3M.4): mesmo molde do XSD. BLOQUEIO: este arquivo é processado antes do bloco de
# third_party/nfse-tabelas em src/mid/CMakeLists.txt, então NL_TABELAS_MANIFESTO_SHA256 ainda não existe aqui e o static_assert de
# dps_validar.cpp para o build. A correção é mover aquele bloco para antes de add_subdirectory(composicao); precisa de autorização do CTO.
target_compile_definitions(nl_mid PRIVATE "NL_TABELAS_MANIFESTO_SHA256=\"${NL_TABELAS_MANIFESTO_SHA256}\"")

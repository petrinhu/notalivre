# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Id da DPS, série, número e chave de acesso mínima (NL-1.3). Fontes no alvo nl_back; cabeçalhos em src/back/include/nl/back/dps/.
target_sources(nl_back PRIVATE
    serie.cpp
    numero_dps.cpp
    id_dps.cpp
    chave_acesso.cpp
)

# NL-1.4: entidade Dps, regime, grupo IBS/CBS e tipos de leiaute (fontes em src/back/include/nl/back/dps/).
target_sources(nl_back PRIVATE
    chave_nfse.cpp
    codigo_classificacao_tributaria.cpp
    codigo_indicador_operacao.cpp
    codigo_justificativa_substituicao.cpp
    codigo_pais.cpp
    codigo_situacao_tributaria.cpp
    codigo_trib_nacional.cpp
    descricao_servico.cpp
    dps.cpp
    ibscbs.cpp
    indicador_destinatario.cpp
    indicador_total_tributos.cpp
    motivo_nao_nif.cpp
    op_simp_nac.cpp
    percentual_tributo.cpp
    percentual_tributo_sn.cpp
    ref_nfse.cpp
    reg_ap_trib_sn.cpp
    reg_esp_trib.cpp
    retencao_issqn.cpp
    telefone.cpp
    tipo_ambiente.cpp
    tipo_operacao.cpp
    trib_issqn.cpp
)

# DA-1.27: data e hora de emissão (dhEmi), com a faixa e o offset do pattern do XSD.
target_sources(nl_back PRIVATE
    dh_emi.cpp
)

# DA-1.17: alíquota do ISSQN e código NBS (formato do XSD).
target_sources(nl_back PRIVATE
    aliquota_issqn.cpp
    codigo_nbs.cpp
)

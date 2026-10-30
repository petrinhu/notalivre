# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Mid/cofre (NL-2.2, DA-2.30): adaptador do A1 que registra o evento de EKU ausente. Acrescenta ao alvo nl_mid.
target_sources(nl_mid PRIVATE adaptador_credencial.cpp)
target_sources(nl_mid PRIVATE material_assinatura.cpp)
# Âncoras de produção do A1 (NL-2.13, DA-2.48): conjunto das raízes da ICP-Brasil, junto do adaptador do A1.
target_sources(nl_mid PRIVATE raizes_producao.cpp)
# O SHA-256 do manifesto fixado entra no alvo do Mid (CISO item 1): só a carga de produção o usa.
target_compile_definitions(nl_mid PRIVATE
                           "NL_ICP_RAIZES_MANIFESTO_SHA256=\"${NL_ICP_RAIZES_MANIFESTO_SHA256}\"")

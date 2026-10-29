# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Mid/cofre (NL-2.2, DA-2.30): adaptador do A1 que registra o evento de EKU ausente. Acrescenta ao alvo nl_mid.
target_sources(nl_mid PRIVATE adaptador_credencial.cpp)
target_sources(nl_mid PRIVATE material_assinatura.cpp)

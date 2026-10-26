# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Identificação fiscal (NL-1.1): fontes acrescentadas ao alvo nl_back, sem alvo próprio. Cabeçalhos públicos em src/back/include/nl/back/identificacao/.
target_sources(nl_back PRIVATE
    cnpj.cpp
    cpf.cpp
    inscricao.cpp
    codigo_municipio.cpp
    cep.cpp
)

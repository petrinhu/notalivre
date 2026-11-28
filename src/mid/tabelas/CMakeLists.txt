# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Catálogo de localidades (NL-3.M, DA-3M.4): adaptador do port CatalogoLocalidades, carregado de arquivos de dados.
# O pino do manifesto NÃO é definido aqui: a carga recebe o pino por parâmetro. A definição do app (NL_TABELAS_MANIFESTO_SHA256,
# vinda do SHA-256 do SHA256SUMS de third_party/nfse-tabelas) entra no passo 1b, junto da fachada que a usa.
target_sources(nl_mid PRIVATE
    catalogo_localidades_embutido.cpp
    carregar_catalogo.cpp)

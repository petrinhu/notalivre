# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes do portão da variável de SHA-1 (NL-2.6, DA-2.10): autoteste com sabotagem e varredura da árvore real.
find_package(Python3 3.11 COMPONENTS Interpreter REQUIRED)

set(NL_TESTE_PORTAO_SHA1 "${CMAKE_CURRENT_SOURCE_DIR}/test_portao_sha1_env.py")

foreach(nome IN ITEMS
    portaoSha1ArvoreRealLimpa
    portaoSha1SabotagemReprova
    portaoSha1ArnesPermitido
    portaoSha1PisoVazioReprova
    portaoSha1ViolacaoEmSrcOutReprova
    portaoSha1ViolacaoEmSrcBuildXReprova
    ciHigieneLigaPortaoSha1)
  add_test(NAME portoes.${nome}
           COMMAND Python3::Interpreter -I "${NL_TESTE_PORTAO_SHA1}" "test_${nome}")
endforeach()

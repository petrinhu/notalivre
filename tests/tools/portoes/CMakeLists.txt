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
    portaoSha1PycDerivadoNaoReprova
    portaoSha1PycSemOrigemReprova
    portaoSha1VazamentoEmThirdPartyReprova
    portaoSha1PastaNovaNaRaizReprova
    portaoSha1PycComSufixoFalsoNaoDerivadoReprova
    portaoSha1PycSemVariavelNaFonteReprova
    portaoSha1MarkdownDocumentalPassa
    portaoSha1WorktreeNaRaizPassa
    portaoSha1WorktreeAninhadoReprova
    portaoSha1WorktreeRastreadoNaRaizReprova
    portaoSha1MarkdownSrcReprova
    portaoSha1TesteXmldsigPermitidoPeloCaminhoExato
    portaoSha1OutroTesteDoXmldsigReprova
    portaoSha1BuildDeclaradoComArgumentoNaoReprova
    portaoSha1BuildVersionadoSemArgumentoReprova
    portaoSha1BuildDeclaradoNaoCegaSrcBuild
    portaoSha1ArgumentoSemCacheProprioRecusado
    portaoSha1ArgumentoCacheDeOutraPastaRecusado
    portaoSha1ArgumentoForaDaRaizRecusado
    ciHigieneLigaPortaoSha1)
  add_test(NAME portoes.${nome}
           COMMAND Python3::Interpreter -I "${NL_TESTE_PORTAO_SHA1}" "test_${nome}")
endforeach()

# Exceção 5 do portão (CISO, DA-2.62 recusada): o CTest passa o CMAKE_BINARY_DIR ao portão só quando o build está DENTRO
# da árvore (Flatpak, cmake -B _build na raiz). Fora da árvore (CI nativo, build_local), o teste roda sem argumento.
string(FIND "${CMAKE_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/" NL_POS_BUILD_NA_ARVORE)
if(NL_POS_BUILD_NA_ARVORE EQUAL 0)
  set_tests_properties(portoes.portaoSha1ArvoreRealLimpa PROPERTIES
                       ENVIRONMENT "NL_BUILD_DIR=${CMAKE_BINARY_DIR}")
endif()

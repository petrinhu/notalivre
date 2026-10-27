# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Testes da NL-1.10 (tools/tabelas): cada caso vira um add_test próprio, rodado com python3 -I.
# O script acha tools/tabelas pelo próprio caminho (RAIZ/tools/tabelas), sem depender de cwd.
find_package(Python3 3.11 COMPONENTS Interpreter REQUIRED)

set(NL_TESTE_TABELAS "${CMAKE_CURRENT_SOURCE_DIR}/test_tabelas.py")

foreach(nome IN ITEMS
    cTribNac10101ViraTexto010101
    cTribNacConfereComItemSubitemDesdobro
    municipioComSeteDigitos
    nbsComFormatoPontuadoPreservado
    cTribNac99XXXXIdentificavel
    nbsSemDescricaoMantidaEContada
    servicoItemSubitemDesdobroInconsistenteFalha
    convSHAErradoSaiComRc1SemGravar
    contagemAlteradaSaiComRc1SemGravar
    conversorFabricadoGravaQuandoBate
    aguasMaritimasConvertida
    xlsxValidoEhLido
    xlsxComDoctypeRecusado
    xlsxComEntityRecusado
    xlsxArquivoDescompactadoGrandeRecusado
    xlsxComArquivosDemaisRecusado
    xlsxUtf16ComEntityRecusado
    xlsxUtf8ComBomLido
    xlsxUtf16SemDoctypeLido
    conversorRecusaXmlComDoctypeRc1)
  add_test(NAME tabelas.${nome}
           COMMAND Python3::Interpreter -I "${NL_TESTE_TABELAS}" ${nome})
endforeach()

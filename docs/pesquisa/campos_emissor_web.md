# Campos do Emissor Web Nacional (NFS-e): mapeamento site x leiaute

Data da coleta: 25/10/2026. Escopo: emissão de DPS pelo Emissor Web Nacional, telas Pessoas, Serviço e Tributação/Valores.

## Fontes

- Scripts públicos (estáticos, lidos localmente, não executados): https://www.nfse.gov.br/EmissorNacional/Scripts/Pessoas, https://www.nfse.gov.br/EmissorNacional/Scripts/Servico e https://www.nfse.gov.br/EmissorNacional/Scripts/Tributacao (arquivos `Pessoas.js`, `Servico.js` e `Tributacao.js`).
- Leiaute oficial: Anexo I da NFS-e Nacional, abas `LEIAUTE DPS_NFS-e` (linhas 1 a 417) e `RN DPS_NFS-e` (regras E).
- Pesquisas internas do projeto (consultadas por busca dirigida, ver seção 11): `docs/pesquisa/ibscbs.md` e `docs/pesquisa/tomador.md`.

## Legenda

- **Tag XML:** caminho completo = `NFSe/infNFSe/DPS/infDPS/` + tag listada. Tags com `IBSCBS/` são do grupo de IBS/CBS da DPS.
- **FATO:** presença do id no script (ou regra lida no código). **[inf]:** inferência por nome ou por lógica, não confirmada no texto do Anexo. Sem marcação, a correspondência é direta pelo nome e pela regra.
- **Obrigatoriedade no site:** só o que o JS determina. Asterisco e `required` vêm do HTML do servidor, que não está nos scripts; quando não consta, está escrito "nao determinavel no JS".
- **Ocorr. (Anexo):** ocorrência do leiaute (`1-1` obrigatório, `0-1` opcional, `CE` condicional de grupo). **Tipo/tam.:** tipo do leiaute (C, N, D, E, CE, G, ID) e tamanho.
- **RN nnn** (nas seções 5 e 6): número da coluna `#` da aba RN DPS_NFS-e; os códigos E da mesma linha seguem entre parênteses.
- **Continuação na aba RN:** linhas sem caminho (`B`/`C` vazios) são atribuídas ao último campo da aba, como continuação da regra. Por isso uma tag pode listar mais de um código E.
- **Regra E:** códigos de regra de negócio cujo caminho é exatamente a tag (busca automática na aba RN; "-" = nenhuma regra com caminho exato; o caminho do grupo não entra).

## 1. Tela Pessoas (Pessoas.js)

| Campo (id) | Tag XML (após NFSe/infNFSe/DPS/infDPS/) | Ocorr. (Anexo) | Tipo/tam. | Obrigatoriedade no site | Depende de / nota | Regra E |
|---|---|---|---|---|---|---|
| `DataCompetencia` | dCompet | 1-1 | D/- | JS: data valida obrigatoria ('data invalida'); >= 01/01/2026 se IBS/CBS ativo | PreencherInfoIBSCBS=1 exige >=2026; troca reinicia Tomador/Intermediario; E0015 dCompet<=dhEmi | E0015, E0016, E1270, E0018... |
| `InformarSerieNumeroDPS` | (UI) | - | - | nenhuma (checkbox) | Mostra pnlTrascricaoDPS; desmarcar limpa SerieDPS/NumeroDPS | - |
| `SerieDPS` | serie | 1-1 | N/1-5 | so visivel com InformarSerieNumeroDPS | Depende de InformarSerieNumeroDPS | E0010, E0014 |
| `NumeroDPS` | nDPS | 1-1 | N/1-15 | so visivel com InformarSerieNumeroDPS | Depende de InformarSerieNumeroDPS | - |
| `TipoEmitente` | tpEmit | 1-1 | N/1 | radio; sempre marcado | tpEmit 2/3 bloqueado pelo site (alerta). RN E9996 rejeita tpEmit 2/3 nesta versao: ALINHADO | E9996 |
| `PreencherInfoIBSCBS` | IBSCBS (presenca do grupo) | 0-1 | -/- | radio; troca limpa a DPS (confirmacao) | =1 exige dCompet>=01/01/2026; abre pnlInfoIBSCBS; [inf] presenca do grupo IBSCBS | E0850, E0854 |
| `Prestador_Inscricao` | prest/CNPJ ou prest/CPF | 1-1 ou 1-1 | N/14 ou N/11 | JS: 11 ou 14 digitos ('CPF/CNPJ invalido') | Dispara consulta de cadastro; cadastro nao encontrado bloqueia | E0080, E0082, E0084, E0096... |
| `Prestador_InscricaoMunicipal` | prest/IM | 0-1 | C/15 | asterisco de lblIMPrestador oculto quando TpDados=='RFB' | Lista vem de ListarEstabelecimentosParaEmissao | E0116, E0119, E0120, E0124 |
| `Prestador_EnderecoNacional_CodigoMunicipio` | prest/end/endNac/cMun | 1-1 | N/7 | select; bloqueado sem emissor habilitado | Depende de DataCompetencia e de Prestador_Inscricao (lista de municipios); alimenta NomeMunicipio e CEP | E0130, E0132, E0134 |
| `Prestador_EnderecoNacional_NomeMunicipio` | (derivado) | - | - | derivado do select | texto do option selecionado; sem tag | - |
| `Prestador_EnderecoNacional_CEP` | prest/end/endNac/CEP | 1-1 | C/8 | JS: 8 digitos ('CEP invalido'); deve pertencer ao municipio | ValidarCepMunicipio (api/EmissaoDPS/Cep) compara CodigoCompletoMunicipio com o municipio selecionado | E0138 |
| `Prestador_EnderecoNacional_Logradouro` | prest/end/xLgr | 1-1 | C/1-255 | nao determinavel no JS | preenchido pelo CEP (TipoLogradouro + Logradouro) | - |
| `Prestador_EnderecoNacional_Numero` | prest/end/nro | 1-1 | C/1-60 | nao determinavel no JS | - | - |
| `Prestador_EnderecoNacional_Complemento` | prest/end/xCpl | 0-1 | C/1-156 | nao determinavel no JS (opcional no Anexo) | - | - |
| `Prestador_EnderecoNacional_Bairro` | prest/end/xBairro | 1-1 | C/1-60 | nao determinavel no JS | preenchido pelo CEP | - |
| `Prestador_EnderecoNacional_IndicadorEnderecoAlterado` | (flag) | - | - | nenhuma | calculado comparando com PrestadorEnderecoOriginal; sem tag (flag interno) | - |
| `Prestador_Nome` | prest/xNome | 0-1 | C/150 | preenchido pelo cadastro (CarregarInfoEmitente) | Vem do cadastro; no Anexo xNome do prest e 0-1 | E0121, E0122, E0123 |
| `Prestador_Telefone` | prest/fone | 0-1 | N/6-20 | opcional | preenchido pelo cadastro | - |
| `Prestador_Email` | prest/email | 0-1 | C/1-80 | opcional | preenchido pelo cadastro | E0148 |
| `Prestador_LocalDomicilio` | (UI) | - | - | radio; default 1 (Brasil) | 1 = mostra endNac; 2 = mostra endExt [inf: 1 Brasil, 2 Exterior, pelo switch do JS]; ver tabela de dependencias | - |
| `SimplesNacional_Opcao` | prest/regTrib/opSimpNac | 1-1 | N/1 | preenchido pelo endpoint RecuperarOpcaoSN | Tipo==0 alerta erro; Tipo==3 mostra regApTribSN [inf: valores 1/2/3 = opSimpNac, confirmar no Anexo] | E0160, E0161 |
| `txtOpcaoSN` | (UI) | - | - | descricao do regime (saida do endpoint) | sem tag: descricao textual de opSimpNac | - |
| `SimplesNacional_RegimeApuracaoTributosSN` | prest/regTrib/regApTribSN | 0-1 | N/1 | visivel so com Tipo==3 | Depende de SimplesNacional_Opcao==3 (ME/EPP); E0230 regEspTrib=0 quando regApTribSN ocorre | E0162, E0166 |
| `Tomador_LocalDomicilio` | (UI) | - | - | radio; 0 = sem identificacao [inf] | 1 = CPF/CNPJ (Brasil); 2 = NIF (exterior); escolhe toma/CNPJ ou toma/CPF, ou toma/NIF | - |
| `Tomador_Inscricao` | toma/CNPJ ou toma/CPF | 1-1 ou 1-1 | N/14 ou N/11 | JS: 11 ou 14 digitos; cadastro nao encontrado bloqueia | Dispara consulta; CNPJ com endereco obrigatorio (ver divergencias) | E0188, E0190, E0194, E0202... |
| `Tomador_InformarEndereco` | (presenca) | - | - | checkbox; JS: CNPJ nao pode desmarcar | presenca do grupo toma/end; ver DIVERGENCIA (site exige sempre para CNPJ; E0234 so condicional) | - |
| `Tomador_EnderecoNacional_CEP` | toma/end/endNac/CEP | 1-1 | C/8 | JS: 8 digitos; BuscarCEP | Depende de Tomador_LocalDomicilio=1 e InformarEndereco | E0240 |
| `Tomador_EnderecoExterior_CodigoPais` | toma/end/endExt/cPais | 1-1 | C/2 | select | Depende de LocalDomicilio=2 | E0246 |
| `Tomador_EnderecoExterior_NomePais` | (derivado) | - | - | derivado do select | sem tag | - |
| `Intermediario_Inscricao` | interm/CNPJ ou interm/CPF | 1-1 ou 1-1 | N/14 ou N/11 | JS: 11 ou 14 digitos | Depende de TipoEmitente=3 (bloqueado pelo site) | E0248, E0250, E0254, E0262... |
| `Intermediario_LocalDomicilio` | (UI) | - | - | radio; default 0 | 1 = CPF/CNPJ; 2 = NIF [inf, mesmo switch do tomador] | - |
| `Intermediario_InformarEndereco` | (presenca) | - | - | checkbox | presenca do grupo interm/end | - |
| `Intermediario_EnderecoExterior_CodigoPais` | interm/end/endExt/cPais | 1-1 | C/2 | select | Depende de Intermediario_LocalDomicilio=2 | E0299 |
| `Intermediario_EnderecoExterior_NomePais` | (derivado) | - | - | derivado do select | sem tag | - |
| `Destinatario_Inscricao` | IBSCBS/dest/CNPJ ou IBSCBS/dest/CPF | 1-1 ou 1-1 | N/14 ou N/11 | JS: CPF/CNPJ de destinatario sem consulta obrigatoria (destinatario nao consulta cadastro) | Depende de DestinatarioEhOAdquirente=0 | E0911, E0912, E0913, E0914 |
| `Destinatario_Nome` | IBSCBS/dest/xNome | 1-1 | C/1-150 | preenchido manualmente | Depende de DestinatarioEhOAdquirente=0 | - |
| `Destinatario_Telefone` | IBSCBS/dest/fone | 0-1 | N/6-20 | opcional | - | - |
| `Destinatario_Email` | IBSCBS/dest/email | 0-1 | C/1-80 | opcional | - | E0930 |
| `Destinatario_InformarEndereco` | (presenca) | - | - | checkbox; JS: para destinatario diferente do adquirente endereco e obrigatorio | presenca do grupo IBSCBS/dest/end | - |
| `Destinatario_EnderecoNacional_CEP` | IBSCBS/dest/end/endNac/CEP | 1-1 | N/8 | JS: 8 digitos | Depende de LocalDomicilio=1 (Brasil) | - |
| `Destinatario_EnderecoNacional_Logradouro` | IBSCBS/dest/end/xLgr | 1-1 | C/1-255 | nao determinavel no JS | preenchido pelo CEP | - |
| `Destinatario_EnderecoNacional_Numero` | IBSCBS/dest/end/nro | 1-1 | C/1-60 | nao determinavel no JS | - | - |
| `Destinatario_EnderecoNacional_Complemento` | IBSCBS/dest/end/xCpl | 0-1 | C/1-156 | opcional | - | - |
| `Destinatario_EnderecoNacional_Bairro` | IBSCBS/dest/end/xBairro | 1-1 | C/1-60 | nao determinavel no JS | preenchido pelo CEP | - |
| `Destinatario_EnderecoNacional_NomeMunicipio` | (derivado) | - | - | derivado de cMun (sem id de select no JS) | cMun do dest/end/endNac nao aparece como id nos scripts [inf] | - |
| `Destinatario_InscricaoMunicipal` | (sem_tag) | - | - | nao determinavel no JS | DIVERGENCIA: o site tem campo; IBSCBS/dest nao tem IM no LEIAUTE | - |
| `DestinatarioEhOAdquirente` | IBSCBS/indDest | 1-1 | N/1 | radio; default 1 (sim) | =0 mostra pnlInfoDestinatario (destinatario identificado) [inf: indDest = 1 - DestinatarioEhOAdquirente; RN 554: dest so se indDest=1] | - |
| `EhCompraGovernamental` | IBSCBS/tpEnteGov (presenca do grupo) | 0-1 | N/1 | radio; =1 exige Tomador CNPJ (alerta) | =1 abre pnlComprasGov e TipoEnteGov; ver divergencias (regra 'CNPJ' sem E-code) | - |
| `TipoEnteGov` | IBSCBS/tpEnteGov | 0-1 | N/1 | select; visivel com EhCompraGovernamental=1 | RN E1600/E1601: gTribCompraGov x tpEnteGov; tpOper obrigatorio se tpEnteGov (E0903) | - |
| `hdfTipoPessoa` | (UI) | - | - | hidden; contexto do modal de historico | sem tag | - |
| `hdfTipoHistorico` | (UI) | - | - | hidden | sem tag | - |
| `btnAvancar` | (UI) | - | - | botao | sem tag | - |
| `btnImportar` | (UI) | - | - | botao do modal de historico | sem tag | - |
| `btnMaisInfoEmitente` | (UI) | - | - | botao; habilitado com RFB | sem tag | - |
| `lblIMPrestador` | (UI) | - | - | asterisco de IM do prestador | sem tag; asterisco oculto se TpDados=='RFB' | - |

## 2. Tela Serviço (Servico.js)

| Campo (id) | Tag XML (após NFSe/infNFSe/DPS/infDPS/) | Ocorr. (Anexo) | Tipo/tam. | Obrigatoriedade no site | Depende de / nota | Regra E |
|---|---|---|---|---|---|---|
| `LocalPrestacao_CodigoPaisPrestacao` | serv/locPrest/cPaisPrestacao | 1-1 | C/2 | select; 'BR' habilita municipio e mostra asterisco (JS) | 'BR' habilita LocalPrestacao_CodigoMunicipioPrestacao; outro pais oculta municipio e limpa incidencia | E0304 |
| `LocalPrestacao_CodigoMunicipioPrestacao` | serv/locPrest/cLocPrestacao | 1-1 | N/7 | asterisco do municipio so com cPais=BR (JS) | Depende de CodigoPaisPrestacao=BR; mudanca reinicia obra, evento e COMEX e recalcula incidencia | E0302, E1402 |
| `LocalPrestacao_DescricaoMunicipioPrestacao` | (derivado) | - | - | derivado do select | sem tag na DPS (xLocPrestacao e saida do infNFSe) | - |
| `LocalPrestacao_NomeMunicipioPrestacao` | (derivado) | - | - | derivado do select | sem tag | - |
| `LocalPrestacao_NomePaisPrestacao` | (derivado) | - | - | derivado do select | sem tag | - |
| `ServicoPrestado_CodigoTributacaoNacional` | serv/cServ/cTribNac | 1-1 | N/6 | habilitado so apos municipio (JS) | Muda: reinicia obra/evento/COMEX, bloqueia Descricao, chama VerificarIncidencia; cTribNac 99.x: nao incidencia/'ParaServico99' | E0310, E0312 |
| `ServicoPrestado_DescricaoCodigoTributacaoNacional` | (derivado) | - | - | derivado do select2 | xTribNac e saida do infNFSe | - |
| `ServicoPrestado_CodigoComplementarMunicipal` | serv/cServ/cTribMun | 0-1 | N/3 | obrigatorio se flag CodigoComplementarMunicipalObrigatorio=='true' (hidden do servidor) | Carrega lista ao escolher cTribNac; alterar dispara VerificarIncidencia | E0314, E0315 |
| `ServicoPrestado_CodigoComplementarMunicipalObrigatorio` | (flag) | - | - | hidden: 'true' torna obrigatorio | flag definido pelo servidor/JS em TratarExibicaoCodigoTributacaoMunicipal | - |
| `ServicoPrestado_CodigoNBS` | serv/cServ/cNBS | 0-1 | N/9 | opcional (habilitado com cTribNac) | Mudanca com InfoTributacao IBS/CBS (hdfPreencherInfoIBSCBS) dispara BuscarListaDeIndOp | E0316, E0318, E0320, E0322 |
| `ServicoPrestado_CodigoIndOp` | IBSCBS/cIndOp | 1-1 | N/6 | bloqueado sem IBS/CBS; obrigatorio com IBS/CBS | cIndOp 20101/20201/20301 ativa Obra_EhObrigatorioImovel (se nao ha obra/evento obrigatorios) | E0901 |
| `ServicoPrestado_CodigoTpOper` | IBSCBS/tpOper | 0-1 | N/1 | bloqueado sem IBS/CBS | 2 ou 3 mostra pnlComprasGovernamentaisDetalhes; E0903: obrigatorio se tpEnteGov ou cTribNac LC 116 (25.05, 15.09, 17.12, 10.05) | E0903, E0904 |
| `ServicoPrestado_ChaveAcessoFornPagRealizado` | (inf) | - | - | campo visivel em pnlComprasGovernamentaisDetalhes (tpOper 2/3) | Nao localizado no LEIAUTE por nome; candidato [inf] IBSCBS/gRefNFSe/refNFSe (nao confirmado) | - |
| `ServicoPrestado_Descricao` | serv/cServ/xDescServ | 1-1 | C/1000 | bloqueado ate cTribNac; obrigatorio (campo 1-1 no Anexo) | Habilitado so depois do cTribNac; limpo ao trocar cTribNac | - |
| `ServicoPrestado_HaExportacaoImunidadeNaoIncidencia` | valores/trib/tribMun/tribISSQN | 1-1 | N/1 | radio; so com municipio de incidencia | 0 = tributavel (mostra pnlMunIncid); 1 = nao tributavel (mostra pnlMotNaoIncid) [inf: 0 -> tribISSQN=1; 1 -> 2/3/4] | E0529, E0530, E0532, E0539... |
| `ServicoPrestado_MotivoNaoTributacao` | valores/trib/tribMun/tribISSQN | 1-1 | N/1 | select visivel com HaExportacao...=1 | Valor 2 mostra pnlTipoImunidade; 3 mostra pnlResultadoExportacao; default = nao incidencia (RN 463: 99.01.01 => tribISSQN=4) [inf: mapa valor->tribISSQN] | E0529, E0530, E0532, E0539... |
| `ServicoPrestado_TipoImunidade` | valores/trib/tribMun/tpImunidade | 0-1 | N/1 | visivel com MotivoNaoTributacao=2 | - | E0592, E0593 |
| `ServicoPrestado_CodigoPaisResultado` | valores/trib/tribMun/cPaisResult | 0-1 | C/2 | hidden flag CodigoPaisResultadoEhObrigatorio | Visivel com MotivoNaoTributacao=3 (exportacao) | E0590, E0591 |
| `ServicoPrestado_CodigoPaisResultadoEhObrigatorio` | (flag) | - | - | hidden: torna cPaisResult obrigatorio | flag do servidor/JS | - |
| `MunicipioIncidencia_Codigo` | (derivado) | - | - | saida do servidor (VerificarIncidencia) | cLocIncid vai no infNFSe (saida), nao na DPS | - |
| `MunicipioIncidencia_Nome` | (derivado) | - | - | saida | sem tag na DPS | - |
| `MunicipioIncidencia_Tipo` | (derivado) | - | - | saida | sem tag na DPS | - |
| `MunicipioIncidencia_EhConveniado` | (derivado) | - | - | saida | sem tag na DPS | - |
| `MunicipioIncidencia_EhConvenioVigente` | (derivado) | - | - | saida | sem tag na DPS | - |
| `Obra_TipoInformacao` | (UI) | - | - | radio: 1 codigo obra, 3 end nacional, 4 end exterior, 5 CIB | escolhe entre serv/obra/cObra, cCIB ou serv/obra/end (JS) | - |
| `Obra_CodigoObra` | serv/obra/cObra | 1-1 | C/1-30 | obrigatorio se Obra_EhObrigatorio e TipoInformacao=1 [inf] | Visivel com TipoInformacao=1; exige local no Brasil (alerta) | - |
| `Obra_CodigoCIB` | serv/obra/cCIB | 1-1 | C/8 | obrigatorio se TipoInformacao=5 [inf] | Visivel com TipoInformacao=5 (CIB) | E0373 |
| `Obra_InscricaoImobiliaria` | serv/obra/inscImobFisc | 0-1 | C/1-30 | opcional | - | - |
| `Obra_EhObrigatorio` | (flag) | - | - | hidden: 'True' torna pnlObraInformada obrigatorio | Definido por VerificarInformacoesComplementaresParaServico e por InformarDadosObra | - |
| `Obra_EhObrigatorioObra` | (flag) | - | - | hidden | Depende de cTribNac (99) e cIndOp | - |
| `Obra_EhObrigatorioImovel` | (flag) | - | - | hidden | cIndOp 20101/20201/20301 e sem obra/evento obrigatorio | - |
| `Evento_TipoInformacao` | (UI) | - | - | radio: 1 identificador, 3 end nacional, 4 end exterior | escolhe atvEvento/idAtvEvt ou atvEvento/end | - |
| `Evento_Identificacao` | serv/atvEvento/idAtvEvt | 1-1 | C/1-30 | obrigatorio se Evento_EhObrigatorio e TipoInformacao=1 [inf] | Visivel com TipoInformacao=1; limpo quando endereco | - |
| `Evento_Descricao` | serv/atvEvento/xNome | 1-1 | C/255 | obrigatorio com evento (campo 1-1 no Anexo) | - | - |
| `Evento_DataInicial` | serv/atvEvento/dtIni | 1-1 | D/- | obrigatorio com evento [inf] | - | - |
| `Evento_DataFinal` | serv/atvEvento/dtFim | 1-1 | D/- | obrigatorio com evento [inf] | - | - |
| `Evento_InformarEndereco` | (presenca) | - | - | checkbox | presenca do grupo atvEvento/end | - |
| `Evento_EhObrigatorio` | (flag) | - | - | hidden | Atividade de evento obrigatoria para cTribNac do item 12 (RN E-linha 357) e para 99 | - |
| `Evento_EhObrigatorioEnderecoNacional` | (flag) | - | - | hidden | flag | - |
| `Evento_CEP` | serv/atvEvento/end/CEP | 1-1 | C/8 | JS: 8 digitos; deve pertencer ao municipio de prestacao | Depende de Evento_TipoInformacao=3 | E0398, E0400 |
| `Evento_Logradouro` | serv/atvEvento/end/xLgr | 1-1 | C/1-255 | nao determinavel no JS | preenchido pelo CEP | - |
| `Evento_Numero` | serv/atvEvento/end/nro | 1-1 | C/1-60 | nao determinavel no JS | - | - |
| `Evento_Complemento` | serv/atvEvento/end/xCpl | 0-1 | C/1-156 | opcional | - | - |
| `Evento_Bairro` | serv/atvEvento/end/xBairro | 1-1 | C/1-60 | nao determinavel no JS | - | - |
| `ComercioExterior_ModoPrestacao` | serv/comExt/mdPrestacao | 1-1 | N/1 | visivel com exportacao (pnlComercioExterior) | - | E0333 |
| `ComercioExterior_VinculoPrestacao` | serv/comExt/vincPrest | 1-1 | N/1 | visivel com exportacao | - | - |
| `ComercioExterior_TipoMoeda` | serv/comExt/tpMoeda | 1-1 | N/3 | visivel com exportacao | - | - |
| `ComercioExterior_ValorServicoMoedaEstrangeira` | serv/comExt/vServMoeda | 1-1 | N/1-15V2 | visivel com exportacao | - | - |
| `ComercioExterior_MecanismoApoioPrestador` | serv/comExt/mecAFComexP | 1-1 | N/2 | visivel com exportacao | - | E0341 |
| `ComercioExterior_MecanismoApoioTomador` | serv/comExt/mecAFComexT | 1-1 | N/2 | visivel com exportacao | - | E0343 |
| `ComercioExterior_MovimentacaoTempBens` | serv/comExt/movTempBens | 1-1 | N/1 | visivel com exportacao | Valor 2 mostra pnlNumeroDI; 3 mostra pnlNumeroRE | E0345 |
| `ComercioExterior_NumeroDI` | serv/comExt/nDI | 0-1 | C/1-12 | visivel com MovimentacaoTempBens=2 | - | E0352, E0354 |
| `ComercioExterior_NumeroRE` | serv/comExt/nRE | 0-1 | C/12 | visivel com MovimentacaoTempBens=3 | - | E0356 |
| `ComercioExterior_CompartilharComMDIC` | serv/comExt/mdic | 1-1 | N/1 | visivel com exportacao [inf] | [inf] mdic = compartilhar com MDIC, pelo nome | - |
| `ExigeGrupoCOMEX` | (flag) | - | - | hidden | flag do servidor | - |
| `Complemento_InformacoesComplementares` | serv/infoCompl/xInfComp | 0-1 | C/2000 | opcional | - | - |
| `Complemento_DocumentoReferencia` | serv/infoCompl/docRef | 0-1 | C/1-255 | opcional | - | - |
| `Complemento_NumeroRespTecnica` | serv/infoCompl/idDocTec | 0-1 | C/1-40 | opcional | - | - |
| `Complemento_Pedido_NumeroPedido` | serv/infoCompl/xPed | 0-1 | C/1-60 | opcional; habilita pnlItensPedido | Depende de: pedido preenchido mostra itens | - |
| `Item` | serv/infoCompl/gItemPed/xItemPed | 1-99 | C/1-60 | itens via modal (pnlItensPedido) | Visivel com pedido; 1-99 itens no Anexo | - |
| `EnderecoExterior_CodigoPais` | (inf) | - | - | select do modal Tomador/Intermediario | [inf] toma/end/endExt/cPais e interm/end/endExt/cPais, conforme o modal | - |
| `EnderecoExterior_NomePais` | (derivado) | - | - | derivado | sem tag | - |
| `EnderecoExterior_PessoaInformarEndereco` | (UI) | - | - | checkbox do modal | presenca de endereco da pessoa | - |
| `EnderecoNacional_PessoaInformarEndereco` | (UI) | - | - | checkbox do modal | presenca de endereco da pessoa | - |
| `InformarDadosObra` | (UI) | - | - | checkbox | presenca do grupo serv/obra; exclusivo com evento (alerta) | - |
| `InformarDadosEvento` | (UI) | - | - | checkbox | presenca do grupo serv/atvEvento; exclusivo com obra (alerta) | - |
| `CodigoComplementarMunicipal` | (UI) | - | - | select2 (lista de cTribMun) | ver ServicoPrestado_CodigoComplementarMunicipal | - |
| `CodigoTributacaoNacional` | (UI) | - | - | select2 (lista de cTribNac) | ver ServicoPrestado_CodigoTributacaoNacional | - |
| `Inscricao` | (UI) | - | - | modal de tomador | sem tag direta | - |
| `Nome` | (UI) | - | - | modal de tomador | sem tag direta | - |
| `LocalDomicilio` | (UI) | - | - | radio do modal de tomador | escolhe Brasil/Exterior | - |
| `InscricaoMunicipal` | (UI) | - | - | campo do modal | toma/IM | - |
| `NIF` | (UI) | - | - | campo do modal | toma/NIF | - |
| `NIFInformado` | (UI) | - | - | radio do modal | toma/NIF ou cNaoNIF | - |
| `MotivoNaoInformacaoNIF` | (UI) | - | - | select do modal | toma/cNaoNIF | - |
| `NumeroNIF` | (UI) | - | - | campo do modal | toma/NIF | - |
| `Email` | (UI) | - | - | campo do modal | toma/email | - |
| `Telefone` | (UI) | - | - | campo do modal | toma/fone | - |
| `CEP` | (UI) | - | - | campo do modal | endNac/CEP | - |
| `Logradouro` | (UI) | - | - | campo do modal | xLgr | - |
| `Numero` | (UI) | - | - | campo do modal | nro | - |
| `Complemento` | (UI) | - | - | campo do modal | xCpl | - |
| `Bairro` | (UI) | - | - | campo do modal | xBairro | - |
| `NomeMunicipio` | (derivado) | - | - | derivado | sem tag | - |
| `CodigoMunicipio` | (UI) | - | - | select do modal | endNac/cMun | - |
| `Endereco` | (UI) | - | - | bloco do modal | sem tag | - |
| `EnderecoBrasil` | (UI) | - | - | bloco | sem tag | - |
| `EnderecoExterior` | (UI) | - | - | bloco | sem tag | - |
| `EnderecoExterior_Bairro` | (UI) | - | - | campo do modal (exterior) | end/xBairro do bloco exterior | - |
| `EnderecoExterior_Logradouro` | (UI) | - | - | campo do modal (exterior) | end/xLgr | - |
| `EnderecoExterior_Numero` | (UI) | - | - | campo do modal (exterior) | end/nro | - |
| `EnderecoExterior_Complemento` | (UI) | - | - | campo do modal (exterior) | end/xCpl | - |
| `EnderecoExterior_Cidade` | (UI) | - | - | campo do modal (exterior) | endExt/xCidade | - |
| `EnderecoExterior_EstadoProvinciaRegiao` | (UI) | - | - | campo do modal (exterior) | endExt/xEstProvReg | - |
| `EnderecoExterior_CodigoEnderecamentoPostal` | (UI) | - | - | campo do modal (exterior) | endExt/cEndPost | - |
| `InformarEndereco` | (UI) | - | - | checkbox do modal | presenca de endereco | - |
| `InformarEnderecoCheck` | (UI) | - | - | bloco | sem tag | - |
| `Inscricao_historico` | (UI) | - | - | botao | sem tag | - |
| `InscricaoExterior` | (UI) | - | - | bloco | sem tag | - |
| `DataCompetenciaFormat` | (UI) | - | - | hidden | sem tag | - |
| `hdfPreencherInfoIBSCBS` | (flag) | - | - | hidden: 'true' habilita campos IBS/CBS | flag do servidor | - |
| `hdfInformadoCodTribMun` | (flag) | - | - | hidden | flag interno | - |
| `hdfNivelInfoTomador` | (flag) | - | - | hidden: 0/1/2/3 = nivel de info do tomador (Tributacao usa) | flag | - |
| `hdfEhCompraGovernamental` | (flag) | - | - | hidden | flag | - |
| `hdfCodigoNBS` | (flag) | - | - | hidden | flag | - |
| `hdfCodigoNacional` | (flag) | - | - | hidden | flag | - |
| `hdfCodigoComplementarMunicipal` | (flag) | - | - | hidden | flag | - |
| `hdfCodMunIncid` | (flag) | - | - | hidden | flag (saida) | - |
| `hdfTomadorNoExterior` | (flag) | - | - | hidden | flag | - |
| `hdfIntermediarioNoExterior` | (flag) | - | - | hidden | flag | - |
| `hdfDescricao` | (flag) | - | - | hidden | flag | - |
| `hdfEhExportacao` | (flag) | - | - | hidden | flag | - |
| `hdfEhSubstituicao` | (flag) | - | - | hidden: substituicao | - | - |
| `hdfCodigoIndOpSubstituida` | (flag) | - | - | hidden | - | - |
| `hdfExigiuGrupoComexNfseSubsituida` | (flag) | - | - | hidden | - | - |
| `hdfEhEmpresaMei` | (flag) | - | - | hidden | flag MEI | - |
| `hdfIdr` | (flag) | - | - | hidden | id de rascunho | - |
| `hdfItensGuid` | (flag) | - | - | hidden | id de itens | - |
| `hdfCodMunEmi` | (flag) | - | - | hidden | flag | - |
| `hdfCMunTomador` | (flag) | - | - | hidden | flag | - |
| `hdfCodMunToma` | (flag) | - | - | hidden | flag | - |
| `tabelaItensPedido` | (UI) | - | - | tabela | sem tag | - |
| `btnIncluirItemPedido` | (UI) | - | - | botao | sem tag | - |
| `btnServicosFavoritos` | (UI) | - | - | botao | sem tag | - |
| `btnFechar` | (UI) | - | - | botao | sem tag | - |
| `btnSalvar` | (UI) | - | - | botao | sem tag | - |
| `btnIncluir` | (UI) | - | - | botao | sem tag | - |
| `btnAlterar` | (UI) | - | - | botao | sem tag | - |
| `btnImportar` | (UI) | - | - | botao | sem tag | - |
| `lblObraImovel` | (UI) | - | - | rotulo | sem tag | - |
| `frmPessoa` | (UI) | - | - | formulario | sem tag | - |
| `select2` | (UI) | - | - | biblioteca | sem tag | - |

## 3. Tela Tributação / Valores (Tributacao.js)

| Campo (id) | Tag XML (após NFSe/infNFSe/DPS/infDPS/) | Ocorr. (Anexo) | Tipo/tam. | Obrigatoriedade no site | Depende de / nota | Regra E |
|---|---|---|---|---|---|---|
| `ISSQN_TributacaoISSQN` | valores/trib/tribMun/tribISSQN | 1-1 | N/1 | select/radio; origem da tributacao | Define quais blocos aparecem (pnlTribMun, pnlOperacaoTributavel) [inf: valor 1 = tributavel] | E0529, E0530, E0532, E0539... |
| `ISSQN_HaSuspensao` | (presenca) | - | - | radio (0/1) | =1 mostra pnlSuspensao; incompativel com HaRetencao=1 (alerta) | - |
| `ISSQN_TipoSuspensaoISSQN` | valores/trib/tribMun/exigSusp/tpSusp | 1-1 | N/1 | obrigatorio se HaSuspensao=1 (tpSusp 1-1) | Visivel com HaSuspensao=1; RN 470 restringe suspensao por decisao judicial/administrativa para certos servicos (texto integral a conferir) | - |
| `ISSQN_NumeroProcesso` | valores/trib/tribMun/exigSusp/nProcesso | 1-1 | C/30 | obrigatorio se HaSuspensao=1 (1-1 no Anexo) | Visivel com HaSuspensao=1; limpo quando HaSuspensao=0 | - |
| `ISSQN_HaRetencao` | (presenca) | - | - | radio (0/1) | =1 mostra pnlRetencao; MEI proibido (alerta); suspensao proibida (alerta); define tpRetISSQN | - |
| `ISSQN_TipoRetencao` | valores/trib/tribMun/tpRetISSQN | 1-1 | N/1 | obrigatorio se HaRetencao=1 (tpRetISSQN 1-1) | 2 = tomador: exige toma com CPF/CNPJ e endereco nacional; 3 = intermediario (E ... tpRetISSQN) | E0580, E0583, E0588, E0594... |
| `ISSQN_HaBeneficioMunicipal` | (presenca) | - | - | radio (0/1) | =1 busca beneficios (BuscarBeneficiosMunicipais*) e mostra pnlBeneficioMunicipal | - |
| `ISSQN_IdBM` | valores/trib/tribMun/BM/nBM | 1-1 | N/14 | select; obrigatorio com beneficio (nBM 1-1) | Carrega RecuperarInfoBeneficio: define se o campo e valor ou percentual | - |
| `ISSQN_ValorMonetarioReducaoBC` | valores/trib/tribMun/BM/vRedBCBM | 0-1 | N/1-15V2 | obrigatorio se flag ..Obrigatorio=='True' (hidden) [inf] | Visivel em pnlBeneficioMunicipalValor [inf: nome do painel] | - |
| `ISSQN_ValorPercentualReducaoBC` | valores/trib/tribMun/BM/pRedBCBM | 0-1 | N/1-3V2 | obrigatorio se flag ..Obrigatorio=='True' (hidden) [inf] | Visivel em pnlBeneficioMunicipalPercentual [inf: nome do painel] | - |
| `ISSQN_ValorMonetarioReducaoBCObrigatorio` | (flag) | - | - | hidden: 'True' torna o valor obrigatorio | definido pelo JS conforme o beneficio | - |
| `ISSQN_ValorPercentualReducaoBCObrigatorio` | (flag) | - | - | hidden | definido pelo JS conforme o beneficio | - |
| `ISSQN_HaDeducaoReducao` | (presenca) | - | - | radio (0/1) | =1 mostra pnlDeducaoReducao; =0 limpa | - |
| `DeducaoReducao_TipoDeducaoReducao` | (UI) | - | - | radio 1 valor / 2 percentual / 3 documentos | 1 -> vDR (pnlDeducaoValor); 2 -> pDR (pnlDeducaoPercentual); 3 -> docDedRed (pnlDeducaoDocumentos) | - |
| `DeducaoReducao_TipoDeducaoReducaoObrigatorio` | (flag) | - | - | hidden: 'SIM'/'NAO' | define obrigatoriedade do tipo | - |
| `DeducaoReducao_ValorMonetario` | valores/vDedRed/vDR | 1-1 | N/1-15V2 | obrigatorio se TipoDeducao=1 [inf] | Visivel com TipoDeducao=1; RN 393: sem deducao em caso de imunidade (tribISSQN de imunidade [inf: valor 3]) | E0446, E0447 |
| `DeducaoReducao_ValorPercentual` | valores/vDedRed/pDR | 1-1 | N/1-3V2 | obrigatorio se TipoDeducao=2 [inf] | Visivel com TipoDeducao=2 | E0453, E0454, E0444 |
| `DeducaoReducao_ValorCalculado` | (derivado) | - | - | calculado | soma dos documentos ou valor | - |
| `ISSQN_BaseDeCalculo` | (derivado) | - | - | calculado pelo site | vBC e saida (infNFSe/valores), nao entra na DPS | - |
| `ISSQN_Valor` | (derivado) | - | - | calculado pelo site | vISSQN e saida | - |
| `ISSQN_spanBaseDeCalculo` | (UI) | - | - | rotulo de span | sem tag | - |
| `ISSQN_spanReducaoBC` | (UI) | - | - | rotulo de span | sem tag | - |
| `ISSQN_Aliquota` | (derivado) | - | - | calculado/parametrizado | pAliq e a aliquota aplicada (saida: pAliqAplic) | - |
| `ISSQN_AliquotaInformada` | valores/trib/tribMun/pAliq | 0-1 | N/1V2 | obrigatorio se AliquotaDeveSerInformada=='True' (hidden) | SN com regApTribSN=1 e retencao=1: aliquota informada obrigatoria (JS) | E0595, E0600, E0602, E0604... |
| `ISSQN_AliquotaDeveSerInformada` | (flag) | - | - | hidden: 'True' torna pAliq obrigatoria | definido pelo JS em HaRetencao | - |
| `ISSQN_RegimeEspecial` | prest/regTrib/regEspTrib | 1-1 | N/1 | select (regime especial) | Lista RegimesEspeciaisQueNaoPermitemRetencao bloqueia retencao [inf: nome; confirmar que e o mesmo campo] | E0172, E0174, E0175, E0177... |
| `TributacaoFederal_PISCofins_SituacaoTributaria` | valores/trib/tribFed/piscofins/CST | 1-1 | N/2 | select; CST 0/vazio oculta o bloco PIS/COFINS | CST 4 ou 6 zera valores e bloqueia aliquotas (JS) | - |
| `TributacaoFederal_PISCofins_TipoRetencao` | valores/trib/tribFed/piscofins/tpRetPisCofins | 0-1 | N/1 | visivel com CST!=0 | - | - |
| `TributacaoFederal_PISCofins_BaseDeCalculo` | valores/trib/tribFed/piscofins/vBCPisCofins | 0-1 | N/1-15V2 | opcional [inf] | Recalcula PIS/COFINS ao mudar | E0677 |
| `TributacaoFederal_PISCofins_AliquotaPIS` | valores/trib/tribFed/piscofins/pAliqPis | 0-1 | N/1-2V2 | bloqueado com CST 4/6 | - | E0686 |
| `TributacaoFederal_PISCofins_AliquotaCOFINS` | valores/trib/tribFed/piscofins/pAliqCofins | 0-1 | N/1-2V2 | bloqueado com CST 4/6 | - | E0692 |
| `TributacaoFederal_PISCofins_ValorPIS` | valores/trib/tribFed/piscofins/vPis | 0-1 | N/1-15V2 | calculado (BC x aliquota) | - | E0694 |
| `TributacaoFederal_PISCofins_ValorCOFINS` | valores/trib/tribFed/piscofins/vCofins | 0-1 | N/1-15V2 | calculado (BC x aliquota) | - | E0696 |
| `TributacaoFederal_ValorCP` | valores/trib/tribFed/vRetCP | 0-1 | N/1-15V2 | opcional | - | E0699 |
| `TributacaoFederal_ValorIRRF` | valores/trib/tribFed/vRetIRRF | 0-1 | N/1-15V2 | opcional | - | E0700 |
| `TributacaoFederal_ValorCSLL` | valores/trib/tribFed/vRetCSLL | 0-1 | N/1-15V2 | opcional | - | E0720, E0724, E0701 |
| `Totais_TipoImpressao` | (UI) | - | - | select 1 reais / 2 percentual | 1 mostra pnlTotaisEmReais; 2 mostra pnlTotaisPercentual | - |
| `Totais_FederalReais` | valores/trib/totTrib/vTotTrib/vTotTribFed | 1-1 | N/1-15V2 | obrigatorio no grupo totTrib (1-1) quando em reais | - | - |
| `Totais_EstadualReais` | valores/trib/totTrib/vTotTrib/vTotTribEst | 1-1 | N/1-15V2 | obrigatorio (1-1) | - | - |
| `Totais_MunicipalReais` | valores/trib/totTrib/vTotTrib/vTotTribMun | 1-1 | N/1-15V2 | obrigatorio (1-1) | - | - |
| `Totais_FederalPercentual` | valores/trib/totTrib/pTotTrib/pTotTribFed | 1-1 | N/1-2V2 | obrigatorio (1-1) quando percentual | - | - |
| `Totais_EstadualPercentual` | valores/trib/totTrib/pTotTrib/pTotTribEst | 1-1 | N/1-2V2 | obrigatorio (1-1) | - | - |
| `Totais_MunicipalPercentual` | valores/trib/totTrib/pTotTrib/pTotTribMun | 1-1 | N/1-2V2 | obrigatorio (1-1) | - | - |
| `ValorTributos_TipoValorTributos` | (UI) | - | - | radio 1 monetario / 2 percentual / 3 nenhum / 4 Simples | 1 -> pnlValoresMonetarios; 2 -> pnlValoresPercentuais; 4 -> pnlValorAliquotaSN; define indTotTrib [inf] | - |
| `ValorTributos_ValorTotalFederal` | valores/trib/totTrib/vTotTrib/vTotTribFed | 1-1 | N/1-15V2 | obrigatorio no modo monetario | alias de Totais_FederalReais [inf] | - |
| `ValorTributos_ValorTotalEstadual` | valores/trib/totTrib/vTotTrib/vTotTribEst | 1-1 | N/1-15V2 | obrigatorio no modo monetario | alias [inf] | - |
| `ValorTributos_ValorTotalMunicipal` | valores/trib/totTrib/vTotTrib/vTotTribMun | 1-1 | N/1-15V2 | obrigatorio no modo monetario | alias [inf] | - |
| `ValorTributos_PercentualTotalFederal` | valores/trib/totTrib/pTotTrib/pTotTribFed | 1-1 | N/1-2V2 | obrigatorio no modo percentual | alias [inf] | - |
| `ValorTributos_PercentualTotalEstadual` | valores/trib/totTrib/pTotTrib/pTotTribEst | 1-1 | N/1-2V2 | obrigatorio no modo percentual | alias [inf] | - |
| `ValorTributos_PercentualTotalMunicipal` | valores/trib/totTrib/pTotTrib/pTotTribMun | 1-1 | N/1-2V2 | obrigatorio no modo percentual | alias [inf] | - |
| `ValorTributos_AliquotaSN` | valores/trib/totTrib/pTotTribSN | 1-1 | N/1-2V2 | obrigatorio no modo Simples (pTotTribSN 1-1) | visivel com TipoValorTributos=4 | E0710 |
| `ValorTributos_CodigoSituacaoTributaria` | IBSCBS/valores/trib/gIBSCBS/CST | 1-1 | N/3 | obrigatorio com IBS/CBS (hdfPreencherInfoIBSCBS) | Muda: limpa grupo, busca classificacoes (BuscarListaDeCClassTrib) | - |
| `ValorTributos_CodigoClassificacaoTributaria` | IBSCBS/valores/trib/gIBSCBS/cClassTrib | 1-1 | N/6 | obrigatorio com IBS/CBS | Depende de CodigoSituacaoTributaria; muda: busca info (BuscarInfoCClassTrib) | E0958, E0959 |
| `ValorTributos_CodigoCreditoPresumido` | IBSCBS/valores/trib/gIBSCBS/cCredPres | 0-1 | N/2 | visivel com PossibilidadeCreditoPresumido | - | - |
| `ValorTributos_ExigeGrupoTributacaoRegular` | (flag) | - | - | derivado da classificacao (SIM/NAO) | =SIM mostra pnlValorIBSCBS_ExigeGrupoTributacaoRegular e exige gTribRegular [inf] | - |
| `ValorTributos_PermiteDiferimento` | (flag) | - | - | derivado da classificacao | =SIM mostra pnlValorIBSCBS_PermiteDiferimento; exige gDif [inf] | - |
| `ValorTributos_PossibilidadeCreditoPresumido` | (flag) | - | - | derivado da classificacao | =SIM mostra pnlValorIBSCBS_PossibilidadeCreditoPresumido; exige cCredPres [inf] | - |
| `ValorTributos_InfoTributacaoRegular_CodigoSituacaoTributariaRegular` | IBSCBS/valores/trib/gIBSCBS/gTribRegular/CSTReg | 1-1 | N/3 | obrigatorio se ExigeGrupoTributacaoRegular=SIM [inf] | Muda: busca classificacoes regulares (BuscarListaDeCClassTrib(true)) | - |
| `ValorTributos_InfoTributacaoRegular_CodigoClassificacaoTributariaRegular` | IBSCBS/valores/trib/gIBSCBS/gTribRegular/cClassTribReg | 1-1 | N/6 | obrigatorio se ExigeGrupoTributacaoRegular=SIM [inf] | Depende do CSTReg | E0969, E0970 |
| `ValorTributos_InfoDiferimento_PercentualDiferimentoCBS` | IBSCBS/valores/trib/gIBSCBS/gDif/pDifCBS | 1-1 | N/1-3V2 | obrigatorio se PermiteDiferimento=SIM [inf] | - | - |
| `ValorTributos_InfoDiferimento_PercentualDiferimentoEstadual` | IBSCBS/valores/trib/gIBSCBS/gDif/pDifUF | 1-1 | N/1-3V2 | obrigatorio se PermiteDiferimento=SIM [inf] | - | - |
| `ValorTributos_InfoDiferimento_PercentualDiferimentoMunicipal` | IBSCBS/valores/trib/gIBSCBS/gDif/pDifMun | 1-1 | N/1-3V2 | obrigatorio se PermiteDiferimento=SIM [inf] | - | - |
| `Valores_ValorServico` | valores/vServPrest/vServ | 1-1 | N/1-15V2 | obrigatorio (vServ 1-1) | Mudanca dispara validacao de descontos; no MEI nao pode alterar (RN 174) | E0427, E0428, E0429 |
| `Valores_ValorDescontoCondicionado` | valores/vDescCondIncond/vDescCond | 0-1 | N/1-15V2 | opcional | - | E0432 |
| `Valores_ValorDescontoIncondicionado` | valores/vDescCondIncond/vDescIncond | 0-1 | N/1-15V2 | opcional | - | E0431 |
| `EnderecoExterior_CodigoPais` | toma/end/endExt/cPais | 1-1 | C/2 | select do modal de pessoa para retencao | Modal de retencao (tomador/intermediario); 'NomePais' e derivado | E0246 |
| `EnderecoExterior_NomePais` | (derivado) | - | - | derivado | sem tag | - |

## 4. Painéis (pnl*): gatilhos

Gatilhos extraídos por parser de proximidade sobre o JS formatado (linha a linha). A atribuição de `case` é por proximidade e pode trocar de switch; ver nota. Painel sem linha de show/hide no parser está listado como "sem gatilho direto no parser" (aparece como alvo de outra função ou como contêiner).

| Tela | Painel | Ações (parser) | Nº de ocorrências | Gatilho principal (handler e case, parser) |
|---|---|---|---|---|
| Pessoas | `pnlComprasGov` | hide | 3 | LiberarFormulario (x1); BloquearFormulario (x1) |
| Pessoas | `pnlDestinatario` | hide | 3 | LiberarFormulario (x1); BloquearFormulario (x1) |
| Pessoas | `pnlDestinatarioEnderecoBrasil` | - | 0 | sem gatilho direto no parser |
| Pessoas | `pnlInfoDestinatario` | slideDown/slideUp | 2 | change.on('input[type=" case "0" (x1); change.on('input[type=" case "1" (x1) |
| Pessoas | `pnlInfoIBSCBS` | slideDown/slideUp | 2 | BloquearFormulario case "1" (x1); BloquearFormulario case "0" (x1) |
| Pessoas | `pnlInscricaoBrasil` | hide/show | 3 | change.on('input[type=" case "1" (x2); change.on('input[type=" case "2" (x1) |
| Pessoas | `pnlInscricaoExterior` | hide/show | 3 | change.on('input[type=" case "1" (x2); change.on('input[type=" case "2" (x1) |
| Pessoas | `pnlIntermediario` | hide/show | 3 | AtualizarPrestador (x1); AtualizarTomador (x1) |
| Pessoas | `pnlIntermediarioEnderecoBrasil` | - | 0 | sem gatilho direto no parser |
| Pessoas | `pnlLimiteSimplesNacional` | slideUp | 1 | BloquearFormulario (x1) |
| Pessoas | `pnlMaisInfo` | slideDown/slideUp | 7 | ListarMunicipiosParaEmissao (x2); click.on( (x2) |
| Pessoas | `pnlPrestador` | hide/show | 3 | AtualizarPrestador (x1); AtualizarTomador (x1) |
| Pessoas | `pnlRegimeApuracaoTributosSN` | hide/show | 2 | RecuperarOpcaoSN (x2) |
| Pessoas | `pnlTomador` | hide/show | 3 | AtualizarPrestador (x1); AtualizarTomador (x1) |
| Pessoas | `pnlTomadorEnderecoBrasil` | - | 0 | sem gatilho direto no parser |
| Pessoas | `pnlTrascricaoDPS` | slideDown/slideUp | 3 | click.on( (x2); change.on( (x1) |
| Servico | `pnlComercioExterior` | slideDown/slideUp | 3 | VerificarCenarioDeExportacao (x2); OcultarPnlCOMEX (x1) |
| Servico | `pnlComprasGovernamentais` | slideDown/slideUp | 2 | ReiniciarPnlComprasGovernamentais (x2) |
| Servico | `pnlComprasGovernamentaisDetalhes` | slideDown/slideUp | 3 | change.on( (x2); ReiniciarPnlComprasGovernamentais (x1) |
| Servico | `pnlEnderecoBrasil` | hide/show | 2 | change.on('input[type=" case "1" (x1); change.on('input[type=" case "2" (x1) |
| Servico | `pnlEnderecoExterior` | hide/show | 2 | change.on('input[type=" case "1" (x1); change.on('input[type=" case "2" (x1) |
| Servico | `pnlEvento` | slideDown/slideUp | 3 | VerificarInformacoesComplementaresParaServico (x2); OcultarPnlEvento (x1) |
| Servico | `pnlEventoDetalhes` | show/slideUp | 2 | ReiniciarPnlEvento (x1); change.on("input[type=radio][id=Evento_TipoInformacao]" (x1) |
| Servico | `pnlEventoEndereco` | show/slideDown/slideUp | 6 | toggleInformarEnderecoEvento (x2); ReiniciarPnlEvento (x1) |
| Servico | `pnlEventoEnderecoExterior` | slideDown/slideUp | 3 | ReiniciarPnlEvento (x1); change.on("input[type=radio][id=Evento_TipoInformacao]" case "3" (x1) |
| Servico | `pnlEventoEnderecoNacional` | show/slideDown/slideUp | 4 | toggleInformarEnderecoEvento (x1); ReiniciarPnlEvento (x1) |
| Servico | `pnlEventoIdentificacao` | slideDown/slideUp | 4 | ReiniciarPnlEvento (x1); change.on("input[type=radio][id=Evento_TipoInformacao]" case "1" (x1) |
| Servico | `pnlEventoInformado` | slideDown/slideUp | 4 | VerificarInformacoesComplementaresParaServico (x2); OcultarPnlEvento (x1) |
| Servico | `pnlEventoParaServico99` | show/slideDown/slideUp | 3 | VerificarInformacoesComplementaresParaServico (x2); ReiniciarPnlEvento (x1) |
| Servico | `pnlInscricaoBrasil` | hide/show | 2 | change.on('input[type=" case "1" (x1); change.on('input[type=" case "2" (x1) |
| Servico | `pnlInscricaoExterior` | hide/show | 2 | change.on('input[type=" case "1" (x1); change.on('input[type=" case "2" (x1) |
| Servico | `pnlItensPedido` | slideDown/slideUp | 2 | ExcluirTodosOsItensPedido (x1); change.on( (x1) |
| Servico | `pnlMotNaoIncid` | slideDown/slideUp | 5 | LimparCamposDeNaoTributacao (x1); TratarNaoIncidencia (x1) |
| Servico | `pnlMsgGrupoCOMEX` | slideDown/slideUp | 5 | change.on("input[type=radio][id=ServicoPrestado_HaExportacao case "1" (x2); change.on( default (x2) |
| Servico | `pnlMsgMunIncidencia` | slideDown/slideUp | 4 | AtualizarMunicipioDeIncidencia (x2); ReiniciarOpcoesAposAlterarCodigoTributacaoNacional (x1) |
| Servico | `pnlMunIncid` | slideDown/slideUp | 6 | TratarNaoIncidencia (x1); ReiniciarOpcoesAposAlterarCodigoTributacaoNacional (x1) |
| Servico | `pnlNumeroDI` | slideDown/slideUp | 4 | change.on( (x3); LimparPnlCOMEX (x1) |
| Servico | `pnlNumeroRE` | slideDown/slideUp | 4 | change.on( (x3); LimparPnlCOMEX (x1) |
| Servico | `pnlObra` | slideDown/slideUp | 2 | VerificarInformacoesComplementaresParaServico (x1); OcultarPnlObra (x1) |
| Servico | `pnlObraCodigoCIB` | slideDown/slideUp | 5 | ReiniciarPnlObra (x1); change.on("input[type=radio][id=Obra_TipoInformacao]" case "1" (x1) |
| Servico | `pnlObraCodigoObra` | slideDown/slideUp | 5 | ReiniciarPnlObra (x1); change.on("input[type=radio][id=Obra_TipoInformacao]" case "1" (x1) |
| Servico | `pnlObraDetalhes` | show/slideUp | 2 | ReiniciarPnlObra (x1); change.on("input[type=radio][id=Obra_TipoInformacao]" (x1) |
| Servico | `pnlObraEndereco` | show/slideDown/slideUp | 5 | ReiniciarPnlObra (x1); change.on("input[type=radio][id=Obra_TipoInformacao]" case "1" (x1) |
| Servico | `pnlObraEnderecoExterior` | slideDown/slideUp | 3 | ReiniciarPnlObra (x1); change.on("input[type=radio][id=Obra_TipoInformacao]" case "3" (x1) |
| Servico | `pnlObraEnderecoNacional` | slideDown/slideUp | 3 | ReiniciarPnlObra (x1); change.on("input[type=radio][id=Obra_TipoInformacao]" case "3" (x1) |
| Servico | `pnlObraInformada` | hide/show/slideDown/slideUp | 8 | VerificarInformacoesComplementaresParaServico (x5); change.on( (x2) |
| Servico | `pnlObraParaServico99` | show/slideDown/slideUp | 4 | VerificarInformacoesComplementaresParaServico (x3); ReiniciarPnlObra (x1) |
| Servico | `pnlResultadoExportacao` | slideDown/slideUp | 6 | LimparCamposDeNaoTributacao (x1); change.on("input[type=radio][id=ServicoPrestado_HaExportacao case "0" (x1) |
| Servico | `pnlResultadoExportacaoPais` | slideDown/slideUp | 3 | VerificarCenarioDeExportacao (x2); LimparCamposDeNaoTributacao (x1) |
| Servico | `pnlServicoPrestado` | - | 0 | sem gatilho direto no parser |
| Servico | `pnlTipoImunidade` | slideDown/slideUp | 6 | LimparCamposDeNaoTributacao (x1); change.on("input[type=radio][id=ServicoPrestado_HaExportacao case "0" (x1) |
| Servico | `pnlTributacaoMunicipal` | slideDown/slideUp | 3 | TratarExibicaoCodigoTributacaoMunicipal (x2); AtualizarMunicipioDeIncidencia (x1) |
| Tributacao | `pnlAliquotaInformada` | slideDown/slideUp | 2 | change.on("input[type=radio][id=ISSQN_HaRetencao]" case "0" (x1); change.on("input[type=radio][id=ISSQN_HaRetencao]" case "1" (x1) |
| Tributacao | `pnlBeneficioMunicipal` | slideDown/slideUp | 7 | BuscarBeneficiosMunicipais (x2); BuscarBeneficiosMunicipaisReducaoBC (x2) |
| Tributacao | `pnlBeneficioMunicipalPercentual` | slideDown/slideUp | 7 | RecuperarInfoBeneficio (x4); LimparCamposDasPerguntas (x1) |
| Tributacao | `pnlBeneficioMunicipalValor` | slideDown/slideUp | 7 | RecuperarInfoBeneficio (x4); LimparCamposDasPerguntas (x1) |
| Tributacao | `pnlComandosPessoas` | - | 0 | sem gatilho direto no parser |
| Tributacao | `pnlDeducaoDocumentos` | slideDown/slideUp | 5 | LimparCamposDasPerguntas (x1); AlterarIndicacaoDeExistenciaDeDedRed case "0" (x1) |
| Tributacao | `pnlDeducaoPercentual` | slideDown/slideUp | 5 | LimparCamposDasPerguntas (x1); AlterarIndicacaoDeExistenciaDeDedRed case "0" (x1) |
| Tributacao | `pnlDeducaoReducao` | slideDown/slideUp | 3 | LimparCamposDasPerguntas (x1); AlterarIndicacaoDeExistenciaDeDedRed case "0" (x1) |
| Tributacao | `pnlDeducaoValor` | slideDown/slideUp | 5 | LimparCamposDasPerguntas (x1); AlterarIndicacaoDeExistenciaDeDedRed case "0" (x1) |
| Tributacao | `pnlDescricao` | slideDown/slideUp | 2 | change.on("#TipoIdentificacaoDeducao" (x2) |
| Tributacao | `pnlDescricaoTipoReembolsoRepasseRessarcimento` | slideDown/slideUp | 2 | change.on("#TipoReembolsoRepasseRessarcimento" (x2) |
| Tributacao | `pnlEndereco` | slideDown/slideUp | 6 | change.on('input[type=" (x2); click.on("#InformarEndereco" (x2) |
| Tributacao | `pnlEnderecoBrasil` | hide/show | 8 | change.on('input[type=" (x8) |
| Tributacao | `pnlEnderecoExterior` | hide/show | 8 | change.on('input[type=" (x8) |
| Tributacao | `pnlFornecedor` | slideDown/slideUp | 4 | click.on("#IdentificarFornecedor" (x4) |
| Tributacao | `pnlInscricaoBrasil` | hide/show | 8 | change.on('input[type=" (x8) |
| Tributacao | `pnlInscricaoExterior` | hide/show | 8 | change.on('input[type=" (x8) |
| Tributacao | `pnlOperacaoTributavel` | - | 0 | sem gatilho direto no parser |
| Tributacao | `pnlPISCOFINS` | slideDown/slideUp | 4 | ExcluirTodosOsDocumentos (x2); change.on("#TributacaoFederal_PISCofins_SituacaoTributaria" (x2) |
| Tributacao | `pnlReembolsoRepasseRessarcimento` | - | 0 | sem gatilho direto no parser |
| Tributacao | `pnlRetencao` | slideDown/slideUp | 3 | LimparCamposDasPerguntas (x1); change.on("input[type=radio][id=ISSQN_HaRetencao]" case "0" (x1) |
| Tributacao | `pnlRetencaoMensagem` | slideDown/slideUp | 2 | VerificarMensagemParaRetencao (x2) |
| Tributacao | `pnlSuspensao` | slideDown/slideUp | 3 | LimparCamposDasPerguntas (x1); change.on("input[type=radio][id=ISSQN_HaSuspensao]" case "0" (x1) |
| Tributacao | `pnlTipoCTe` | hide/show | 2 | change.on("#TipoChaveDocumentoFiscalEletronico" (x1); change.on("#TipoChaveDocumentoFiscalEletronico" case "3" (x1) |
| Tributacao | `pnlTipoNFNFS` | hide/show | 2 | change.on("#TipoDocumentoDeducao" (x1); change.on("#TipoDocumentoDeducao" case "4" (x1) |
| Tributacao | `pnlTipoNFSe` | hide/show | 4 | change.on("#TipoDocumentoDeducao" (x1); change.on("#TipoDocumentoDeducao" case "2" (x1) |
| Tributacao | `pnlTipoNFe` | hide/show | 4 | change.on("#TipoDocumentoDeducao" (x1); change.on("#TipoDocumentoDeducao" case "1" (x1) |
| Tributacao | `pnlTipoOutrasNFSe` | hide/show | 2 | change.on("#TipoDocumentoDeducao" (x1); change.on("#TipoDocumentoDeducao" case "3" (x1) |
| Tributacao | `pnlTipoOutro` | hide/show | 2 | change.on("#TipoChaveDocumentoFiscalEletronico" (x1); change.on("#TipoChaveDocumentoFiscalEletronico" case "9" (x1) |
| Tributacao | `pnlTipoOutroDocFiscal` | hide/show | 3 | change.on('input[type=" (x2); change.on("#TipoChaveDocumentoFiscalEletronico" (x1) |
| Tributacao | `pnlTipoOutroDocNaoFiscal` | hide/show | 3 | change.on('input[type=" (x2); change.on("#TipoChaveDocumentoFiscalEletronico" (x1) |
| Tributacao | `pnlTipoOutrosDocumentos` | hide/show | 2 | change.on("#TipoDocumentoDeducao" (x1); change.on("#TipoDocumentoDeducao" case "6" (x1) |
| Tributacao | `pnlTipoOutrosDocumentosFiscais` | hide/show | 2 | change.on("#TipoDocumentoDeducao" (x1); change.on("#TipoDocumentoDeducao" case "5" (x1) |
| Tributacao | `pnlTotaisEmReais` | hide/show | 2 | change.on("#Totais_TipoImpressao" case "1" (x1); change.on("#Totais_TipoImpressao" case "2" (x1) |
| Tributacao | `pnlTotaisPercentual` | hide/show | 2 | change.on("#Totais_TipoImpressao" case "1" (x1); change.on("#Totais_TipoImpressao" case "2" (x1) |
| Tributacao | `pnlTotaisValores` | slideDown | 1 | change.on("#Totais_TipoImpressao" case "2" (x1) |
| Tributacao | `pnlTotal` | - | 0 | sem gatilho direto no parser |
| Tributacao | `pnlTribMun` | - | 0 | sem gatilho direto no parser |
| Tributacao | `pnlTribMunValores` | slideDown | 1 | change.on("input[type=radio][id=ISSQN_HaRetencao]" case "0" (x1) |
| Tributacao | `pnlTributacaoFederal` | - | 0 | sem gatilho direto no parser |
| Tributacao | `pnlTributacaoMunicipal` | slideUp | 1 | click.on("#btnFechar" (x1) |
| Tributacao | `pnlValorAliquotaSN` | slideDown/slideUp | 4 | change.on('input[type=" case "1" (x1); change.on('input[type=" case "2" (x1) |
| Tributacao | `pnlValorIBSCBS_ExigeGrupoTributacaoRegular` | slideDown/slideUp | 3 | LimparIBSCBS (x1); BuscarInfoCClassTrib (x1) |
| Tributacao | `pnlValorIBSCBS_PermiteDiferimento` | slideDown/slideUp | 3 | LimparIBSCBS (x1); BuscarInfoCClassTrib (x1) |
| Tributacao | `pnlValorIBSCBS_PossibilidadeCreditoPresumido` | slideDown/slideUp | 3 | LimparIBSCBS (x1); BuscarInfoCClassTrib (x1) |
| Tributacao | `pnlValorTributos` | - | 0 | sem gatilho direto no parser |
| Tributacao | `pnlValoresMonetarios` | slideDown/slideUp | 4 | change.on('input[type=" case "1" (x1); change.on('input[type=" case "2" (x1) |
| Tributacao | `pnlValoresPercentuais` | slideDown/slideUp | 4 | change.on('input[type=" case "1" (x1); change.on('input[type=" case "2" (x1) |

Nota de atribuição: o handler e o `case` citados são os últimos vistos antes da linha; o parser zera o `case` ao trocar de função ou de handler. Ainda assim, em switch aninhado a atribuição pode estar errada: conferir no JS da URL pública antes de usar como regra de negócio.

## 5. Árvore de dependências

Convenção: `gatilho -> efeito -> campos que ficam obrigatórios ou dependentes -> regra E / endpoint`. FATO = lido no JS ou na regra; [inf] = inferência.

1. **TipoEmitente (tpEmit)** -> 1 (prestador) é o único habilitado; 2 e 3 são bloqueados por alerta. -> RN E9996 rejeita tpEmit 2 e 3 nesta versão (ALINHADO).
2. **DataCompetencia (dCompet)** -> data inválida bloqueia o formulário (BloquearFormulario); com PreencherInfoIBSCBS=1, data antes de 01/01/2026 é recusada; trocar a data limpa Tomador e Intermediário e recarrega municípios (`ListarMunicipiosParaEmissao`). -> E0850 (IBS/CBS a partir de 2026) ALINHADO.
3. **Prestador_Inscricao + dCompet -> prest/end/endNac/cMun -> prest/IM** -> o município carrega a lista de inscrições municipais (`ListarEstabelecimentosParaEmissao`); no ramo `TpDados=='RFB'` o asterisco de IM é ocultado e a info cadastral é carregada (`RecuperarInfoEstabelecimento`). O ramo não-RFB não foi lido integralmente.
4. **SimplesNacional_Opcao (retorno de RecuperarOpcaoSN)** -> Tipo 0 mostra alerta de erro; Tipo 3 mostra `prest/regTrib/regApTribSN` (SimplesNacional_RegimeApuracaoTributosSN). [inf] Tipo corresponde a opSimpNac.
5. **MEI (opSimpNac=2) -> retenção do ISSQN proibida** (site: alerta em ISSQN_HaRetencao=1 com usuarioMei). -> RN 491 (E0583) (tpRetISSQN deve ser 1) ALINHADO.
6. **Simples + regApTribSN=1 + ISSQN_HaRetencao=1 -> pAliq obrigatória** (FATO: pnlAliquotaInformada e AliquotaDeveSerInformada='True'). [inf] Confirmar a regra E equivalente na RN.
7. **LocalDomicilio (prest, tomador, intermediário, destinatário)** -> valor 1 mostra InscricaoBrasil e EnderecoBrasil e oculta o exterior; valor 2 mostra InscricaoExterior, força InformarEndereco e destrava campos de endereço exterior; no tomador e no intermediário, valor 0 retrai o painel (FATO). Escolhe entre `CNPJ/CPF` e `NIF` no tomador.
8. **Tomador_Inscricao com 14 dígitos (CNPJ) -> endereço do tomador obrigatório** (FATO: o site impede desmarcar InformarEndereco). -> DIVERGÊNCIA D1.
9. **NIFInformado (0/1)** -> 0 mostra `MotivoNaoInformacaoNIF` (cNaoNIF) e oculta NIF; 1 mostra NIF (FATO).
10. **EhCompraGovernamental=1 -> TipoEnteGov e pnlComprasGov** -> exige tomador CNPJ (alerta) -> DIVERGÊNCIA D3. Servico: `tpOper` 2 ou 3 mostra `ChaveAcessoFornPagRealizado`; E0903 torna tpOper obrigatório quando há tpEnteGov ou cTribNac da LC 116 (25.05, 15.09, 17.12, 10.05); E1600/E1601 ligam gTribCompraGov a tpEnteGov (ALINHADO).
11. **PreencherInfoIBSCBS=1 -> pnlInfoIBSCBS e hdfPreencherInfoIBSCBS='true'** -> habilita `IBSCBS/cIndOp`, `tpOper` (Servico) e a classificação CST/cClassTrib (Tributação); sem IBS/CBS esses campos ficam bloqueados (FATO).
12. **LocalPrestacao_CodigoPaisPrestacao = BR** -> habilita `cLocPrestacao` com asterisco; outro país oculta o município e limpa a incidência (FATO). E0304: cPais diferente de BR (ALINHADO).
13. **cLocPrestacao 0000000 (Águas Marítimas) + cTribNac 200101** -> alerta de incidência (art. 3º, §3º da LC 116) (FATO, `VerificarIncidenciaDoImposto`). RN 312 (E0302)/313 (ALINHADO).
14. **cTribNac começa com 99** -> pnl de obra e evento "ParaServico99" e TratarNaoIncidencia (FATO). MotivoNaoTributacao só aceita 99.01.01 (alerta); RN 463 (E0532): 99.01.01 implica tribISSQN=4 (ALINHADO).
15. **cTribNac do item 12 da lista** -> atividade de evento obrigatória (RN 357 (E0390); mecanismo no site = flag `Evento_EhObrigatorio` do servidor [inf]).
16. **cIndOp em 20101, 20201 ou 20301** -> `Obra_EhObrigatorioImovel='true'` se não houver obra ou evento obrigatório (FATO).
17. **Obra e Evento são mutuamente exclusivos** (alerta FATO em InformarDadosObra e InformarDadosEvento). RN não verificada.
18. **HaExportacaoImunidadeNaoIncidencia** -> 0 mostra pnlMunIncid (tributável); 1 mostra pnlMotNaoIncid (não tributável) (FATO). Em COMEX: MovimentacaoTempBens 2 mostra nDI; 3 mostra nRE (FATO).
19. **ISSQN_HaSuspensao=1 x ISSQN_HaRetencao=1** -> mutuamente exclusivos (alerta FATO). RN de exclusão não encontrada nos termos buscados.
20. **ISSQN_HaRetencao=1, TipoRetencao 2 (tomador) ou 3 (intermediário)** -> a pessoa precisa ser identificada com endereço no município de incidência (alertas FATO por `hdfNivelInfoTomador`/`hdfNivelInfoIntermediario` 0, 1 ou 3). RN 156 (E0031), 157 (E0032), 239 (E0204) (CPF/CNPJ do tomador), 258 (E0234): ALINHADO, com a ressalva de D1.
21. **ISSQN_HaBeneficioMunicipal=1 -> BuscarBeneficiosMunicipais (ou ReducaoBC)** -> `ISSQN_IdBM` -> RecuperarInfoBeneficio decide entre `vRedBCBM` (campo valor) e `pRedBCBM` (campo percentual) (FATO quanto aos painéis).
22. **ISSQN_HaDeducaoReducao=1 -> TipoDeducaoReducao** -> 1 mostra valor (`vDR`); 2 mostra percentual (`pDR`); 3 mostra documentos (`docDedRed`, 1 a 1000) (FATO). RN 393 (E0435): sem dedução em imunidade (ALINHADO quanto à regra).
23. **PIS/COFINS: CST** -> 0 ou vazio oculta o bloco; 4 ou 6 zera os valores e bloqueia alíquotas; diferente disso mostra `pnlPISCOFINS` e chama o tipo de retenção (FATO).
24. **Totais_TipoImpressao (1 reais, 2 percentual)** e **ValorTributos_TipoValorTributos (1 monetário, 2 percentual, 4 Simples)** -> escolhem entre `vTotTrib*` e `pTotTrib*` (FATO). `totTrib` é 1-1 no Anexo.
25. **IBS/CBS: CodigoSituacaoTributaria -> lista de classificações -> CodigoClassificacaoTributaria -> info** -> mostra ExigeGrupoTributacaoRegular, PermiteDiferimento e PossibilidadeCreditoPresumido (FATO quanto aos painéis; obrigatoriedade dos campos de cada um é [inf]).
26. **Valores_ValorServico** -> dispara revalidação dos descontos; no MEI, vServ não pode ser alterado depois (RN 174 (E0061), ALINHADO quanto à regra).

## 6. Divergências site x Anexo I

- **D1. Endereço do tomador CNPJ.** Site: obrigatório para qualquer CNPJ de tomador (impede desmarcar InformarEndereco). Anexo: E0234 exige o endereço só quando a incidência for no local do tomador (planilha MUN.INCID_INFO.SERV.) ou quando cIndOp está na lista (030102, 050102, 100101, 100301, 100501, 030103, 050103, 100102, 100201, 100302, 100401, 100502, 100601). **Site mais restritivo que o Anexo.** Decisão de produto (líder).
- **D2. Inscrição municipal do destinatário.** Site: `Destinatario_InscricaoMunicipal` existe. Anexo: o grupo `IBSCBS/dest` não tem IM. **Campo do site sem tag no leiaute.**
- **D3. Compras governamentais exige tomador CNPJ.** Site: alerta "precisa ser CNPJ" quando EhCompraGovernamental=1. Busca na aba RN por "compras governamentais" com "CNPJ" sem resultado nos termos buscados. **Regra só do site (não confirmada).**
- **D4. Mesmo CPF/CNPJ em mais de um papel.** Tomador x prestador: E0202 compara o CNPJ completo (ALINHADO com o site). Intermediário x prestador: E0262 compara só a RAIZ (8 primeiros dígitos); o site compara o número completo. **Divergência: o site é menos restritivo** (aceita mesma raiz com CNPJ diferente, que o Anexo rejeita). Tomador x intermediário: o site bloqueia; nenhuma regra E encontrada nos termos buscados ("tomador" com "intermediário" e "igual"). **Regra só do site (não confirmada).**
- **D5. Campo `ChaveAcessoFornPagRealizado` sem tag identificada.** Candidato [inf] `IBSCBS/gRefNFSe/refNFSe`, não confirmado.
- **Sem divergência (alinhados):** tpEmit 2 e 3 bloqueados (E9996); dCompet de IBS/CBS a partir de 2026 (E0850); MEI sem retenção (RN 491 (E0583)); CEP pertencente ao município (RN 211 (E0138), 260 (E0240)); cPais diferente de BR quando exterior (RN 314 (E0304)); 99.01.01 com tribISSQN=4 (RN 463 (E0532)).

## 7. Endpoints internos citados (não acessados, L-57)

Só o nome e os parâmetros lidos no JS. Nenhum foi chamado.

- `api/emissaodps/recuperaropcaosn` (GET: tpInsc, insc, dtCompet)
- `api/emissaodps/ListarMunicipiosParaEmissao` (GET: tpInsc, insc, dtCompet)
- `api/emissaodps/ListarEstabelecimentosParaEmissao` (GET: tpInsc, insc, codMun, dtCompet)
- `api/emissaodps/RecuperarInfoEstabelecimento` (GET: tpInsc, insc, codMun, inscMun, dtCompet)
- `api/EmissaoDPS/Cep/{cep}` (GET; cabeçalho Authorization lido de sessionStorage; compara CodigoCompletoMunicipio)
- `DPS/Pessoas/ModalHistoricoPessoas/` (POST: tpPessoa, tpHistorico)
- `api/emissaodps/cep/{cep}` (Servico, BuscarCepParaEventoEObra)
- `api/emissaodps/VerificarIncidencia` (GET; parâmetros em `data` não lidos integralmente)
- `api/emissaodps/BuscarNomeMunicipio` e `api/emissaodps/BuscarNomeServico` (select2, delay 500)
- `api/emissaodps/RecuperarListaDeIndOp` (parâmetros não lidos integralmente)
- `DPS/Pedido/ExcluirTodos/`, `DPS/Pedido/ModalInclusao/`, `DPS/Pedido/ModalEdicao/`, `DPS/Pedido/Excluir/`
- `DPS/Servico/ModalServicosFavoritos/` (POST) e `dps/modaltomador/abrir/` (GET)
- `api/emissaodps/recuperarbeneficiosvigentes`, `api/emissaodps/recuperarbeneficiosvigentesreducaobc`, `api/emissaodps/recuperarbeneficio`, `api/emissaodps/verificarretencoes` (GET)
- `api/emissaodps/RecuperarListaDeCClassTrib` e `api/emissaodps/RecuperarInfoCClassTrib` (GET)
- `dps/modalPessoaParaRetencao/abrir/` (GET)
- `DPS/DeducaoDocumento/{ModalInclusao, ModalEdicao, Excluir, ExcluirTodos}` e `DPS/ReembolsoRepasseRessarcimento/{ModalInclusao, ModalEdicao, Excluir, ExcluirTodos}`

## 8. Cobertura reversa: campos do Anexo sem campo no site

Folhas do leiaute DPS (tipos E e CE, com ocorrência e tamanho): 247. Sem nenhum id mapeado: 134. Lista (tag, tipo, ocorrência, tamanho):

- `id` (C, 1-1, 45)
- `tpAmb` (N, 1-1, 1)
- `dhEmi` (D, 1-1, -)
- `verAplic` (C, 1-1, 1-20)
- `cMotivoEmisTI` (N, 0-1, 1)
- `chNFSeRej` (N, 0-1, 50)
- `cLocEmi` (N, 1-1, 7)
- `substchSubstda` (C, 1-1, 50)
- `substcMotivo` (N, 1-1, 1)
- `substxMotivo` (C, 0-1, 15-255)
- `prest/NIF` (C, 1-1, 40)
- `prest/cNaoNIF` (N, 1-1, 1)
- `prest/CAEPF` (N, 0-1, 14)
- `prest/end/endExt/cPais` (C, 1-1, 2)
- `prest/end/endExt/cEndPost` (C, 1-1, 1-11)
- `prest/end/endExt/xCidade` (C, 1-1, 1-60)
- `prest/end/endExt/xEstProvReg` (C, 1-1, 1-60)
- `toma/NIF` (C, 1-1, 40)
- `toma/cNaoNIF` (N, 1-1, 1)
- `toma/CAEPF` (N, 0-1, 14)
- `toma/IM` (C, 0-1, 15)
- `toma/xNome` (C, 1-1, 150)
- `toma/end/endNac/cMun` (N, 1-1, 7)
- `toma/end/endExt/cEndPost` (C, 1-1, 1-11)
- `toma/end/endExt/xCidade` (C, 1-1, 1-60)
- `toma/end/endExt/xEstProvReg` (C, 1-1, 1-60)
- `toma/end/xLgr` (C, 1-1, 1-255)
- `toma/end/nro` (C, 1-1, 1-60)
- `toma/end/xCpl` (C, 0-1, 1-156)
- `toma/end/xBairro` (C, 1-1, 1-60)
- `toma/fone` (N, 0-1, 6-20)
- `toma/email` (C, 0-1, 1-80)
- `interm/NIF` (C, 1-1, 40)
- `interm/cNaoNIF` (N, 1-1, 1)
- `interm/CAEPF` (N, 0-1, 14)
- `interm/IM` (C, 0-1, 15)
- `interm/xNome` (C, 1-1, 150)
- `interm/end/endNac/cMun` (N, 1-1, 7)
- `interm/end/endNac/CEP` (C, 1-1, 8)
- `interm/end/endExt/cEndPost` (C, 1-1, 1-11)
- `interm/end/endExt/xCidade` (C, 1-1, 1-60)
- `interm/end/endExt/xEstProvReg` (C, 1-1, 1-60)
- `interm/end/xLgr` (C, 1-1, 1-255)
- `interm/end/nro` (C, 1-1, 1-60)
- `interm/end/xCpl` (C, 0-1, 1-156)
- `interm/end/xBairro` (C, 1-1, 1-60)
- `interm/fone` (N, 0-1, 6-20)
- `interm/email` (C, 0-1, 1-80)
- `serv/cServ/cIntContrib` (C, 0-1, 20)
- `serv/obra/end/CEP` (C, 1-1, 8)
- `serv/obra/end/endExt/cEndPost` (C, 1-1, 1-11)
- `serv/obra/end/endExt/xCidade` (C, 1-1, 1-60)
- `serv/obra/end/endExt/xEstProvReg` (C, 1-1, 1-60)
- `serv/obra/end/xLgr` (C, 1-1, 1-255)
- `serv/obra/end/nro` (C, 1-1, 1-60)
- `serv/obra/end/xCpl` (C, 0-1, 1-156)
- `serv/obra/end/xBairro` (C, 1-1, 1-60)
- `serv/atvEvento/end/endExt/cEndPost` (C, 1-1, 11)
- `serv/atvEvento/end/endExt/xCidade` (C, 1-1, 1-60)
- `serv/atvEvento/end/endExt/xEstProvReg` (C, 1-1, 1-60)
- `valores/vServPrest/vReceb` (N, 0-1, 1-15V2)
- `valores/vDedRed/documentos/docDedRed/chNFSe` (N, 1-1, 50)
- `valores/vDedRed/documentos/docDedRed/chNFe` (N, 1-1, 44)
- `valores/vDedRed/documentos/docDedRed/NFSeMun/cMunNFSeMun` (N, 1-1, 7)
- `valores/vDedRed/documentos/docDedRed/NFSeMun/nNFSeMun` (N, 1-1, 15)
- `valores/vDedRed/documentos/docDedRed/NFSeMun/cVerifNFSeMun` (C, 1-1, 9)
- `valores/vDedRed/documentos/docDedRed/NFNFS/nNFS` (N, 1-1, 7)
- `valores/vDedRed/documentos/docDedRed/NFNFS/modNFS` (N, 1-1, 15)
- `valores/vDedRed/documentos/docDedRed/NFNFS/serieNFS` (C, 1-1, 9)
- `valores/vDedRed/documentos/docDedRed/nDocFisc` (C, 1-1, 255)
- `valores/vDedRed/documentos/docDedRed/nDoc` (C, 1-1, 255)
- `valores/vDedRed/documentos/docDedRed/tpDedRed` (N, 1-1, 2)
- `valores/vDedRed/documentos/docDedRed/xDescOutDed` (C, 0-1, 150)
- `valores/vDedRed/documentos/docDedRed/dtEmiDoc` (D, 1-1, -)
- `valores/vDedRed/documentos/docDedRed/vDedutivelRedutivel` (N, 1-1, 1-15V2)
- `valores/vDedRed/documentos/docDedRed/vDeducaoReducao` (N, 1-1, 1-15V2)
- `valores/vDedRed/documentos/docDedRed/fornec/CNPJ` (N, 1-1, 14)
- `valores/vDedRed/documentos/docDedRed/fornec/CPF` (N, 1-1, 11)
- `valores/vDedRed/documentos/docDedRed/fornec/NIF` (C, 1-1, 40)
- `valores/vDedRed/documentos/docDedRed/fornec/cNaoNIF` (N, 1-1, 1)
- `valores/vDedRed/documentos/docDedRed/fornec/CAEPF` (N, 0-1, 14)
- `valores/vDedRed/documentos/docDedRed/fornec/IM` (C, 0-1, 15)
- `valores/vDedRed/documentos/docDedRed/fornec/xNome` (C, 1-1, 150)
- `valores/vDedRed/documentos/docDedRed/fornec/end/endNac/cMun` (N, 1-1, 7)
- `valores/vDedRed/documentos/docDedRed/fornec/end/endNac/CEP` (C, 1-1, 8)
- `valores/vDedRed/documentos/docDedRed/fornec/end/endExt/cPais` (C, 1-1, 2)
- `valores/vDedRed/documentos/docDedRed/fornec/end/endExt/cEndPost` (C, 1-1, 1-11)
- `valores/vDedRed/documentos/docDedRed/fornec/end/endExt/xCidade` (C, 1-1, 1-60)
- `valores/vDedRed/documentos/docDedRed/fornec/end/endExt/xEstProvReg` (C, 1-1, 1-60)
- `valores/vDedRed/documentos/docDedRed/fornec/xLgr` (C, 1-1, 1-255)
- `valores/vDedRed/documentos/docDedRed/fornec/nro` (C, 1-1, 1-60)
- `valores/vDedRed/documentos/docDedRed/fornec/xCpl` (C, 0-1, 1-156)
- `valores/vDedRed/documentos/docDedRed/fornec/xBairro` (C, 1-1, 1-60)
- `valores/vDedRed/documentos/docDedRed/fornec/fone` (N, 0-1, 6-20)
- `valores/vDedRed/documentos/docDedRed/fornec/email` (C, 0-1, 1-80)
- `valores/trib/totTrib/indTotTrib` (N, 1-1, 1)
- `IBSCBS/finNFSe` (N, 1-1, 1)
- `IBSCBS/indFinal` (N, 0-1, 1)
- `IBSCBS/gRefNFSe/refNFSe` (C, 1-99, 50)
- `IBSCBS/dest/NIF` (C, 1-1, 40)
- `IBSCBS/dest/cNaoNIF` (N, 1-1, 1)
- `IBSCBS/dest/end/endNac/cMun` (N, 1-1, 7)
- `IBSCBS/dest/end/endExt/cPais` (C, 1-1, 2)
- `IBSCBS/dest/end/endExt/cEndPost` (C, 1-1, 1-11)
- `IBSCBS/dest/end/endExt/xCidade` (C, 1-1, 1-60)
- `IBSCBS/dest/end/endExt/xEstProvReg` (C, 1-1, 1-60)
- `IBSCBS/imovel/inscImobFisc` (C, 0-1, 1-30)
- `IBSCBS/imovel/cCIB` (C, 1-1, 8)
- `IBSCBS/imovel/end/CEP` (N, 1-1, 8)
- `IBSCBS/imovel/end/endExt/cEndPost` (C, 1-1, 1-11)
- `IBSCBS/imovel/end/endExt/xCidade` (C, 1-1, 1-60)
- `IBSCBS/imovel/end/endExt/xEstProvReg` (C, 1-1, 1-60)
- `IBSCBS/imovel/end/xLgr` (C, 1-1, 1-255)
- `IBSCBS/imovel/end/nro` (C, 1-1, 1-60)
- `IBSCBS/imovel/end/xCpl` (C, 0-1, 1-156)
- `IBSCBS/imovel/end/xBairro` (C, 1-1, 1-60)
- `IBSCBS/valores/gReeRepRes/documentos/dFeNacional/tipoChaveDFe` (N, 1-1, 1)
- `IBSCBS/valores/gReeRepRes/documentos/dFeNacional/xTipoChaveDFe` (C, 0-1, 1-255)
- `IBSCBS/valores/gReeRepRes/documentos/dFeNacional/chaveDFe` (C, 1-1, 1-50)
- `IBSCBS/valores/gReeRepRes/documentos/docFiscalOutro/cMunDocFiscal` (N, 1-1, 7)
- `IBSCBS/valores/gReeRepRes/documentos/docFiscalOutro/nDocFiscal` (C, 1-1, 1-255)
- `IBSCBS/valores/gReeRepRes/documentos/docFiscalOutro/xDocFiscal` (C, 1-1, 1-255)
- `IBSCBS/valores/gReeRepRes/documentos/docOutro/nDoc` (C, 1-1, 1-255)
- `IBSCBS/valores/gReeRepRes/documentos/docOutro/xDoc` (C, 1-1, 1-255)
- `IBSCBS/valores/gReeRepRes/documentos/fornec/CNPJ` (N, 1-1, 14)
- `IBSCBS/valores/gReeRepRes/documentos/fornec/CPF` (N, 1-1, 11)
- `IBSCBS/valores/gReeRepRes/documentos/fornec/NIF` (C, 1-1, 40)
- `IBSCBS/valores/gReeRepRes/documentos/fornec/cNaoNIF` (N, 1-1, 1)
- `IBSCBS/valores/gReeRepRes/documentos/fornec/xNome` (C, 1-1, 1-150)
- `IBSCBS/valores/gReeRepRes/documentos/dtEmiDoc` (D, 1-1, -)
- `IBSCBS/valores/gReeRepRes/documentos/dtCompDoc` (D, 1-1, -)
- `IBSCBS/valores/gReeRepRes/documentos/tpReeRepRes` (N, 1-1, 2)
- `IBSCBS/valores/gReeRepRes/documentos/xTpReeRepRes` (C, 0-1, 0-150)
- `IBSCBS/valores/gReeRepRes/documentos/vlrReeRepRes` (N, 1-1, 1-15V2)

Nota: esta lista mostra o que o Anexo pede e o site não expõe com id próprio. Parte é preenchida pelo servidor, pelo cadastro ou por valor fixo (ex.: tpAmb, verAplic, dhEmi), e não é falha de cobertura do mapeamento. Exige leitura do fluxo de envio (fora do escopo deste mapeamento).

## 9. Gate de aceite (contagens)

Contagens calculadas por script sobre os três `.js` originais (regex `#[A-Z][A-Za-z0-9]*_[A-Za-z0-9_]+` para ids de aceite, excluídos `btn_*`; regex `#pnl[A-Za-z0-9_]+` para painéis).

| Tela | Ids de aceite (encontrados) | Ids mapeados no documento | Não mapeados | Painéis (encontrados) | Painéis no documento | Painéis na ordem de serviço |
|---|---|---|---|---|---|---|
| Pessoas | 39 | 39 | 0 | 16 | 16 | 16 |
| Servico | 62 | 62 | 0 | 36 | 36 | 36 |
| Tributacao | 66 | 66 | 0 | 48 | 48 | 46 |

Ids de aceite que não entraram na tabela de mapeamento (lista vazia = nenhum):
- Pessoas: nenhum
- Servico: nenhum
- Tributacao: nenhum

Divergências de contagem em relação à ordem de serviço: Pessoas 39 ids e 16 painéis (confere); Serviço 62 ids e 36 painéis (confere); Tributação 66 ids (confere) e **48 painéis, e não 46** (diferença de 2, não explicada pela leitura; os 48 são todos `#pnl*` com sufixo não vazio). Os números reais são os desta tabela.

Ids adicionais fora do regex de aceite (sem underscore, ou atributos `[id$=...]`/`getElementById`) estão mapeados nas tabelas das telas como itens extras, por exemplo `DataCompetencia`, `TipoEmitente`, `NumeroDPS`, `SerieDPS`, `TipoEnteGov`, `EhCompraGovernamental`, `PreencherInfoIBSCBS`. Variantes genéricas de atributo (ex.: `Endereco`, `NIF`, `Bairro`, `CEP`, `Telefone`) aparecem nos seletores `[id$=...]` dos painéis; foram tratadas como família (bloco de modal), não como campo isolado.

## 10. O que não foi mapeado com certeza

- Obrigatoriedade real de cada campo: os asteriscos e `required` vêm do HTML do servidor, não dos scripts. Só a obrigatoriedade pelo JS (alerta, flag `*Obrigatorio` ou show do asterisco) está indicada.
- Rótulos visíveis dos campos: os scripts não contêm os textos do formulário, só mensagens de alerta.
- Parâmetros de `VerificarIncidencia`, `RecuperarListaDeIndOp` e de vários endpoints de modal: não lidos integralmente.
- Mapa valor -> tribISSQN de `ServicoPrestado_MotivoNaoTributacao` e `HaExportacaoImunidadeNaoIncidencia`: inferido do switch e da RN 463, não confirmado nos valores do Anexo.
- Semântica de `LocalDomicilio` 0, 1 e 2 e de `TipoEmitente` como rótulos: inferida do switch.
- Ramo não-RFB de `ListarEstabelecimentosParaEmissao` (obrigatoriedade de IM): não lido integralmente.
- Regras de exclusão retenção x suspensão e de Obra x Evento: alerta do site confirmado, E-code do Anexo não localizado nos termos buscados.

## 11. Cruzamento com docs/pesquisa (ibscbs.md, tomador.md)

- **indDest** (`ibscbs.md`, linha 18): 0 = destinatário é o tomador; 1 = não é. **Confirma** a inversão indicada na tabela (DestinatarioEhOAdquirente=1 corresponde a indDest=0).
- **tpEnteGov** (`ibscbs.md`, linha 17): 1 União, 2 Estado, 3 DF, 4 Município; só em compras governamentais. Confirma que TipoEnteGov é o campo certo; a lista de opções do select não foi lida nos scripts.
- **E0234** (`tomador.md`, linha 34): endereço do tomador obrigatório só quando a incidência cai no local do tomador ou quando cIndOp está na lista. **Confirma** o alinhamento da seção 5 (item 20) e a divergência D1.
- **D1, reforço** (`tomador.md`, linha 47): a própria pesquisa registra como INFERÊNCIA que a leitura de E0235 sugere que o Anexo não exige endereço nacional do tomador CNPJ de forma geral. Isso reforça a divergência D1 (o site exige sempre).
- **IM do tomador** (`tomador.md`, linha 23): grupo toma tem IM 0-1. **Confirma** o campo do site para o tomador.
- **IM do destinatário**: `ibscbs.md` e `tomador.md` não registram IM no grupo `dest`. **Confirma** a divergência D2.
- **Consulta de cadastro de tomador** (`tomador.md`, linha 7): o manual não lista endpoint de consulta cadastral de tomador por CPF/CNPJ. Coerente com a leitura dos scripts (as consultas feitas pelo site são de endpoints internos, não acessados).

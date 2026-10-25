# Prompt para LLM: Projeto de App Emissor de NFS-e Nacional (API Oficial do Governo)

## Contexto e Objetivo

Você é um engenheiro de software sênior especializado em sistemas fiscais brasileiros. Sua tarefa é criar um aplicativo emissor de NFS-e (Nota Fiscal de Serviço Eletrônica) usando **exclusivamente a API oficial do Sistema Nacional de NFS-e** do governo brasileiro (SEFIN Nacional / ADN).

O aplicativo deve:
1. Funcionar primeiro no **ambiente de Produção Restrita** (testes/homologação)
2. Permitir troca fácil para o ambiente de Produção quando validado
3. Usar autenticação via certificado digital ICP-Brasil (mTLS)
4. Ser robusto, seguro e fácil de manter

## Ambientes Oficiais

| Ambiente | Finalidade | Validade Fiscal | URL da API (SEFIN) |
|----------|------------|-----------------|---------------------|
| **Produção Restrita** | Testes e homologação | **Não** (notas fictícias) | `https://sefin.producaorestrita.nfse.gov.br/SefinNacional` |
| **Produção** | Notas reais | **Sim** | `https://sefin.nfse.gov.br/SefinNacional` |

**Swagger / Documentação:**
- Produção Restrita: `https://sefin.producaorestrita.nfse.gov.br/API/SefinNacional/docs/index`
- Produção: `https://sefin.nfse.gov.br/SefinNacional/docs/index`

**Página oficial com todos os endpoints:**
https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/apis-prod-restrita-e-producao

**Emissor Web de testes (para emissão manual e comparação):**
https://www.producaorestrita.nfse.gov.br/EmissorNacional

## Autenticação (Importante)

A API oficial **não usa OAuth**, token Bearer, usuário/senha ou gov.br.

- Autenticação obrigatória: **mTLS** com certificado digital ICP-Brasil (A1 ou A3)
- O certificado deve ser do tipo **e-CNPJ** (ou e-CPF em casos específicos de procuração)
- O XML da DPS deve ser assinado digitalmente com **XMLDSIG**
- O certificado usado na conexão mTLS e na assinatura do XML deve ser o mesmo (ou do mesmo CNPJ raiz)

**Não existe** forma de autenticar na API usando login de conta CNPJ ou gov.br. Esses métodos servem apenas para o Emissor Web no navegador.

## Requisitos Técnicos da Integração

1. **Protocolo**: HTTPS com TLS 1.2+ e autenticação mútua (mTLS)
2. **Formato de envio**: JSON contendo o XML da DPS compactado em GZip + Base64
3. **Assinatura**: XMLDSIG (enveloped) no XML da DPS
4. **Ambiente**: Controlado por configuração (Produção Restrita = testes, Produção = real)
5. **Endpoints principais**:
   - `POST /nfse` → Recebe DPS e retorna NFS-e autorizada (ou erros)
   - `GET /nfse/{chaveAcesso}` → Consulta NFS-e
   - `GET /dps/{id}` → Recupera chave de acesso a partir do ID da DPS

## Escopo do Projeto

Crie um aplicativo desktop com as seguintes funcionalidades mínimas:

### Fase 1 — Ambiente de Testes (Produção Restrita)
- [ ] Configuração de certificado A1 (.pfx/.p12) + senha
- [ ] Seleção de ambiente (Produção Restrita / Produção) via config
- [ ] Geração de XML DPS válido conforme leiaute nacional
- [ ] Assinatura digital do XML (XMLDSIG)
- [ ] Compactação GZip + Base64
- [ ] Envio via mTLS para `POST /nfse`
- [ ] Tratamento de erros retornados pela API
- [ ] Consulta de NFS-e gerada
- [ ] Logs detalhados para debug

### Fase 2 — Melhorias
- [ ] Interface para preenchimento dos dados da nota (prestador, tomador, serviço, valores) e todos os citados oficialmente
- [ ] Geração de DANFSe (PDF) quando disponível
- [ ] Cancelamento de NFS-e
- [ ] Histórico local de emissões
- [ ] Validação prévia do XML contra schema XSD
- [ ] Criação de templates/modelos pré-preenchidos

## Restrições e Boas Práticas

- **Nunca** hardcode certificado ou senha no código
- Usar variáveis de ambiente ou arquivo de configuração seguro
- Sempre testar primeiro em Produção Restrita
- Tratar corretamente a cadeia de certificados ICP-Brasil
- Implementar retry com backoff para falhas transitórias de rede
- Registrar (log) o ID da DPS e a chave de acesso retornada
- Seguir as regras de numeração da DPS (série + número sequencial por prestador)

## Documentação Oficial de Referência

- Portal NFS-e: https://www.gov.br/nfse/
- Documentação técnica atual: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/documentacao-atual
- Documentação de homologação/testes: https://www.gov.br/nfse/pt-br/biblioteca/documentacao-tecnica/producao-restrita
- Manual de Contribuintes (API): buscar a versão mais recente no portal

## Stack Sugerida (você pode escolher outra)

Preferências em ordem de praticidade para mTLS + XMLDSIG (discuta antes com o usuário e dê outras opcoes com prós e contras, usando askuserquestion)
1. **Python** (requests + cryptography / signxml)
2. **Node.js** (axios + node-forge ou xml-crypto)
3. **C# / .NET** (HttpClientHandler + SignedXml)
4. **Java** (OkHttp / Apache HttpClient + XMLSignature)

## Entregáveis Esperados

1. Código fonte organizado e comentado
2. Arquivo de configuração de exemplo (`.env.example` ou equivalente)
3. README com instruções de:
   - Como obter e configurar o certificado A1
   - Como rodar em Produção Restrita
   - Como migrar para Produção
   - Principais erros comuns e como resolver
4. Exemplo de DPS mínima válida para teste

## Observações Finais

- Notas emitidas em **Produção Restrita não têm validade fiscal**.
- O leiaute pode evoluir (acompanhe as Notas Técnicas no portal oficial).
- Em caso de dúvida sobre campos específicos da DPS, consulte o Anexo I da documentação técnica mais recente.
- Priorize a correção da autenticação mTLS e da assinatura XML antes de investir em interface gráfica.

---

**Instrução final para a LLM:**
Comece pelo ambiente de Produção Restrita. Implemente primeiro o fluxo mínimo (certificado → DPS assinada → POST /nfse → resposta). Só depois adicione interface e funcionalidades extras. Documente claramente como trocar de ambiente.

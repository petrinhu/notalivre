# Mapa de ativação (Cósimo, opus/high, 25/10/2026): transcrição fiel

PRÉ-REQUISITO: TODO.md ausente; gerar via /tab_pendencias depois do brainstorm.

PORTE: early
VARIANTE: Pipeline-Early com régua de criticidade elevada (ORG.md:129, pipeline_release_1.0.md:413: Narciso e Cláudio ativos quando early mexe com dinheiro/PII). Domínio fiscal regulado; A1 assina em nome do CNPJ; nota em Produção é ato fiscal irreversível; PII de tomadores (LGPD).
C-LEVELS ATIVOS: Celso (go/no-go), Caetano (arquitetura/engenharia), Narciso (custódia certificado/senha, mTLS, XMLDSIG, credenciais SMTP), Cláudio (LGPD, licença do repo público, responsabilidade fiscal).
C-LEVELS DORMENTES: Capitolino (CONDICIONAL: acorda se houver distribuição a terceiros), Camilo, Cosmo, Cândido, Confúcio, Cícero, Caio.
AGENTS OPERACIONAIS: software-architect, backend-engineer, frontend-engineer, security-engineer, compliance-legal, devops-sre, qa-engineer, technical-writer.
  + ADICIONADO pelo orquestrador por ordem do líder ("acione a skill de frontend para fazer um app moderno e bonito"): visual-design-director.
FASES: 0-2 colapsadas (prompt-emissor-nfse-nacional.md = one-pager); 3 wireframe leve; 4 (ADRs assinatura/mTLS, custódia do certificado, banco), 5 (CI matriz Fedora/Ubuntu/Manjaro/Mint) e 8 não colapsam; 6-7 contínuas; 9 = dogfooding em Produção Restrita; 10 dormente; 11-12 enxutas.
CERIMÔNIA: nenhuma; kanban implícito na TODO.md.
GATILHO DE RE-ROTEAMENTO: (a) chegada do A1 e 1ª emissão em Produção Restrita; (b) virada para Produção (classe L-70); (c) distribuição a terceiros, vários prestadores ou monetização.
JUSTIFICATIVA ANTI-OE: app desktop de um líder, sem receita/mercado/time; só a criticidade justifica Narciso e Cláudio.
VALIDAÇÃO: AC1 = A1 (prompt-emissor-nfse-nacional.md:57). Conflito de canon: linhas 92-98 sugerem Python/Node/.NET/Java vs verbatim "qt[latest]" (L-66/L-67: resolver com o líder). MySQL externo pesado para desktop (opinião do agente).
DÚVIDAS QUE MUDAM O PORTE: 1 uso próprio × distribuição; 2 vários prestadores/CNPJs (escritório contábil); 3 monetização/suporte; 4 volume do lote/sem supervisão.

# RE-ROTEAMENTO (Cósimo, opus/high, 25/10/2026): transcrição fiel resumida
Gatilho (c) disparado. PORTE: early. VARIANTE: Pipeline-Lean (pipeline_release_1.0.md:409; ORG.md:123), criticidade elevada mantida.
ATIVOS: Celso, Caetano, Capitolino (acorda), Narciso (regime sobe: A1 de terceiros em máquinas não controladas), Cláudio. Camilo CONDICIONAL light na Fase 10.
DORMENTES: Cosmo, Confúcio, Cícero, Cândido, Caio.
OPERACIONAIS: anteriores + visual-design-director + product-manager + ux-writer + accessibility-specialist light. Dormentes até resposta sobre suporte: support-engineer, customer-success, release-manager, community-manager.
FASES: 0-3 leves mas explícitas; 4, 5, 8 reais; 6-7 contínuas; 9 beta com terceiros; 10 light; 11-12 enxutas. Cerimônia: kanban na TODO.md.
GATILHO: sobe para scale com escritório contábil/multi-CNPJ, monetização/suporte contratual, telemetria/nuvem, ou base real de usuários. 1ª release que deixa terceiro emitir em Produção: go/no-go do Celso com assinatura de Narciso e Cláudio.
ACRÉSCIMOS: tensão MySQL local × Flatpak; Qt LGPLv3 embutido gera obrigações LGPL (licença/EULA com Cláudio); LGPD (INFERÊNCIA): prestador que instala = controlador; autor = fornecedor, salvo telemetria/sync/suporte com acesso; produto precisa exportação/exclusão/retenção (art. 18).
DÚVIDAS: persona; monetização/suporte; formato; quem mantém MySQL; telemetria/atualização automática/nuvem.

# Respostas do líder, rodada 2 (25/10/2026)
Formato = "só flatpak"; MySQL = "Sim, com assistente"; camadas = "Núcleo + CLI + interface"; público = "MEI e autônomos, Pequenas empresas" (escritórios contábeis NÃO).

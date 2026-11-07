# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
# Cota de núcleos do espelho e da sabotagem (NL-3.0k, ordem do CTO de 07/11/2026). Átomo único: o padrão e a regra de
# validação moram aqui. Carregado por ponto, com caminho relativo ao próprio script: `. .../cota_nucleos.sh`.
# Não é executável. Chamadores: tools/espelho/job_flatpak.sh e tools/mutacao/sabotar_portoes.sh (modo flatpak).

# Cota padrão, em núcleos. Igual ao THREADS de tools/conteiner/build_local.sh. A máquina é dividida: 7 de 16 para este projeto.
NL_COTA_PADRAO=7

# nl_threads_valido VALOR NPROC: imprime a cota efetiva em stdout e sai 0; sai 1 com REPROVA em stderr se for inválida.
# VALOR vazio vale NL_COTA_PADRAO. Só passa inteiro positivo sem zero à esquerda, até NPROC-1 (um núcleo fica ao sistema).
# De NL_COTA_PADRAO+1 até NPROC-1 passa com AVISO: acima da cota exige ok do GlintFx (memória feedback_divisao_maquina_glintfx).
# Valor com mais de 4 dígitos reprova antes da comparação: a aritmética do bash estoura em número longo e daria verde falso.
nl_threads_valido() {
    local valor="${1:-}" np="${2:?uso: nl_threads_valido VALOR NPROC}" teto
    [ -n "$valor" ] || valor="$NL_COTA_PADRAO"
    teto=$((np - 1))
    if ! [[ "$valor" =~ ^[1-9][0-9]*$ ]]; then
        echo "REPROVA: NL_THREADS=$valor não é inteiro positivo sem zero à esquerda (ex.: NL_THREADS=7)" >&2
        return 1
    fi
    if [ "${#valor}" -gt 4 ] || [ "$valor" -gt "$teto" ]; then
        echo "REPROVA: NL_THREADS=$valor acima do teto de $teto núcleos (nproc $np, um fica ao sistema)" >&2
        return 1
    fi
    if [ "$valor" -gt "$NL_COTA_PADRAO" ]; then
        echo "AVISO: NL_THREADS=$valor acima da cota de $NL_COTA_PADRAO núcleos; exige ok do GlintFx" >&2
    fi
    echo "$valor"
}

// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Adaptador do port CatalogoLocalidades (NL-3.M, DA-3M.4). Lê os três JSON do Anexo A de um diretório e confere, nesta
// ordem: o SHA256SUMS contra o pino recebido, o SHA-256 de cada JSON contra o manifesto, e só então faz o parse. O catálogo
// é carregado uma vez; as consultas são em memória, sem I/O (contrato DA-1.25).
#ifndef NL_MID_TABELAS_CATALOGO_LOCALIDADES_EMBUTIDO_H
#define NL_MID_TABELAS_CATALOGO_LOCALIDADES_EMBUTIDO_H

#include <nl/back/ports/catalogo_localidades.h>
#include <nl/base/erro.h>

#include <QSet>
#include <QString>

#include <expected>
#include <string_view>

namespace nl::mid::tabelas {

class CatalogoLocalidadesEmbutido final : public nl::back::CatalogoLocalidades
{
public:
    /// Carrega o catálogo de `diretorio`. `pinoManifesto` é o SHA-256 esperado do SHA256SUMS, em 64 hex minúsculos.
    /// Erros (códigos estáveis; a mensagem não traz caminho de arquivo nem conteúdo do JSON):
    ///  - "tabelas.ausente": diretório, SHA256SUMS ou um dos três JSON não abre;
    ///  - "configuracao.tabelas_manifesto_nao_fixado": o SHA256SUMS não é o do pino (erro de configuração);
    ///  - "tabelas.manifesto_invalido": linha fora do formato sha256sum, arquivo repetido ou ausente do manifesto;
    ///  - "tabelas.arquivo_divergente": o SHA-256 de um JSON não confere com o manifesto;
    ///  - "tabelas.json_invalido": JSON que não faz parse, sem a lista "registros" ou com código fora do domínio.
    static std::expected<CatalogoLocalidadesEmbutido, nl::base::Erro> carregar(const QString& diretorio,
                                                                               std::string_view pinoManifesto);

    bool municipioExiste(const nl::back::CodigoMunicipio& codigo) const override;
    bool paisExiste(const nl::back::CodigoPais& codigo) const override;

private:
    CatalogoLocalidadesEmbutido(QSet<QString> municipios, QSet<QString> paises);

    // Municípios de municipios.json mais as localidades de localidades.json (0000000, Águas Marítimas).
    QSet<QString> municipios_;
    // Países ISO2 de paises.json.
    QSet<QString> paises_;
};

} // namespace nl::mid::tabelas

#endif // NL_MID_TABELAS_CATALOGO_LOCALIDADES_EMBUTIDO_H

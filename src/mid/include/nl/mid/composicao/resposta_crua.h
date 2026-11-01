// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Resposta crua da Sefin para análise local (NL-2.9, DA-2.59 item 4; NL-2.10c, DA-2.66). A fachada grava cada resposta HTTP da
// emissão, recusas incluídas, em arquivo 0600 (uma linha JSON por resposta), por allowlist (CISO N-03): só a chave sai mascarada
// (# no lugar dos caracteres) e cada outro valor sai como <tipo:tamanho>; os nomes e a forma dos campos ficam. Nome do arquivo é
// o hash do Id, sem o Id nem a chave. Nenhuma senha passa por aqui.
#ifndef NL_MID_COMPOSICAO_RESPOSTA_CRUA_H
#define NL_MID_COMPOSICAO_RESPOSTA_CRUA_H

#include <nl/base/erro.h>
#include <nl/mid/sefin/cliente/observador_respostas.h>

#include <QByteArray>
#include <QString>

#include <expected>
#include <optional>
#include <string>
#include <string_view>

namespace nl::mid::composicao {

/// Corpo com a PII mascarada: nfseXmlGZipB64 vira marcador, chaveAcesso vira #. Corpo que não é JSON vira um aviso.
QByteArray mascararRespostaCrua(const QByteArray& corpo);

/// Observador da emissão (NL-2.10c, DA-2.66): grava uma linha JSON por resposta HTTP da Sefin em
/// <pastaBase>/respostas/<hash do Id>.jsonl, com método, rota (modelo), status, Content-Type e o corpo mascarado pela allowlist.
/// Erro de gravação não desfaz a emissão: fica em ultimoErro(), e quem chama decide o que mostrar.
class GravadorRespostaCrua final : public nl::mid::sefin::cliente::ObservadorRespostas {
public:
    GravadorRespostaCrua(QString pastaBase, std::string idDps);

    void registrar(std::string_view metodo, std::string_view rota, int status, const QByteArray& tipoConteudo,
                   const QByteArray& corpo) override;

    /// Arquivo do Id. Vazio enquanto nada foi gravado.
    [[nodiscard]] const QString& caminho() const { return caminho_; }
    /// Primeiro erro de gravação, se houve.
    [[nodiscard]] const std::optional<nl::base::Erro>& ultimoErro() const { return erro_; }

private:
    QString pastaBase_;
    std::string idDps_;
    QString caminho_;
    std::optional<nl::base::Erro> erro_;
};

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_RESPOSTA_CRUA_H

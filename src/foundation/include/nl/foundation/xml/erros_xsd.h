// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Erro da validação XSD (NL-1.6a): motivo enumerado mais a lista de erros de nl::base::Erro, nunca vazia.
#ifndef NL_FOUNDATION_XML_ERROS_XSD_H
#define NL_FOUNDATION_XML_ERROS_XSD_H

#include <nl/base/erro.h>

#include <vector>

namespace nl::foundation::xml {

/// Falha de validação. A lista de erros detalha cada ocorrência; o construtor garante que nunca fica vazia.
class ErrosXsd
{
public:
    enum class Motivo {
        xmlMalformado,   ///< o XML nem chega a ser lido (erro de sintaxe)
        esquemaRecusado, ///< o esquema não confere (SHA-256 divergente, arquivo ausente ou não compila)
        invalido,        ///< o XML é bem formado e não confere com o esquema
        falhaInterna     ///< a própria libxml2 falhou (memória, contexto), não é culpa do documento
    };

    ErrosXsd(Motivo motivo, std::vector<nl::base::Erro> erros);

    Motivo motivo() const noexcept { return motivo_; }
    const std::vector<nl::base::Erro>& erros() const noexcept { return erros_; }

private:
    Motivo motivo_;
    std::vector<nl::base::Erro> erros_;
};

} // namespace nl::foundation::xml

#endif // NL_FOUNDATION_XML_ERROS_XSD_H

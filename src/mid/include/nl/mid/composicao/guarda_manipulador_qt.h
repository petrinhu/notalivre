// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Guarda RAII do manipulador do Qt (NL-2.9, item 4 da triagem do CISO). Instala o manipulador ligado ao Log ao entrar no
// escopo e, ao sair (retorno normal ou exceção), instala o que descarta, nunca o padrão do Qt. Move-only não é preciso: o
// registro é emprestado e a guarda não é copiável.
#ifndef NL_MID_COMPOSICAO_GUARDA_MANIPULADOR_QT_H
#define NL_MID_COMPOSICAO_GUARDA_MANIPULADOR_QT_H

namespace nl::foundation::log {
class Log;
}

namespace nl::mid::composicao {

class GuardaManipuladorQt
{
public:
    /// Instala o manipulador apontado para o registro. O registro tem de viver mais que a guarda.
    explicit GuardaManipuladorQt(nl::foundation::log::Log* registro);
    GuardaManipuladorQt(const GuardaManipuladorQt&) = delete;
    GuardaManipuladorQt& operator=(const GuardaManipuladorQt&) = delete;
    ~GuardaManipuladorQt();
};

} // namespace nl::mid::composicao

#endif // NL_MID_COMPOSICAO_GUARDA_MANIPULADOR_QT_H

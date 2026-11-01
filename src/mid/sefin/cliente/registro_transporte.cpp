// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Evento sefin.transporte_falhou (NL-2.10e, E4, DA-2.75). Ver registro_transporte.h.
#include <nl/mid/sefin/cliente/registro_transporte.h>

namespace nl::mid::sefin::cliente {

// STUB do passo vermelho (NL-2.10e): ainda não registra nada, para o teste falhar por asserção.
void registrarTransporteFalhou(nl::foundation::log::Log& registro, nl::foundation::transporte::CausaErro causa, int codigoQt,
                               bool handshake, qint64 bytesEscritos, std::string_view etapa)
{
    static_cast<void>(registro);
    static_cast<void>(causa);
    static_cast<void>(codigoQt);
    static_cast<void>(handshake);
    static_cast<void>(bytesEscritos);
    static_cast<void>(etapa);
}

} // namespace nl::mid::sefin::cliente

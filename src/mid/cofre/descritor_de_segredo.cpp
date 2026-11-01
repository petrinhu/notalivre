// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Canal de segredo por descritor (NL-2.27, DA-2.16). Ver descritor_de_segredo.h.
#include <nl/mid/cofre/descritor_de_segredo.h>

namespace nl::mid::cofre {

// Stub do NL-2.27, passo 2 (commit de teste): ainda não lê. Todo descritor é recusado como inválido. Os testes de leitura falham por
// asserção; o teste de descritor inválido passa e é declarado como guarda.
std::expected<nl::foundation::pkcs12::SenhaSegura, ErroDoSegredo> lerSenhaDoDescritor(int /*fd*/)
{
    return std::unexpected(ErroDoSegredo::DescritorInvalido);
}

} // namespace nl::mid::cofre

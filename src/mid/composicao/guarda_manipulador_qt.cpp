// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Guarda RAII do manipulador do Qt (NL-2.9, item 4). Ver guarda_manipulador_qt.h. Stub de teste: o corpo real vem no código.
#include <nl/mid/composicao/guarda_manipulador_qt.h>

namespace nl::mid::composicao {

GuardaManipuladorQt::GuardaManipuladorQt(nl::foundation::log::Log* registro)
{
    static_cast<void>(registro);
}

GuardaManipuladorQt::~GuardaManipuladorQt() = default;

} // namespace nl::mid::composicao

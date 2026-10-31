// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
// Guarda RAII do manipulador do Qt (NL-2.9, item 4). Ver guarda_manipulador_qt.h. Ao sair, desliga com o registro nulo, que
// instala o descarte (commit 2124048), nunca o padrão do Qt.
#include <nl/mid/composicao/guarda_manipulador_qt.h>

#include <nl/mid/composicao/manipulador_qt.h>

namespace nl::mid::composicao {

GuardaManipuladorQt::GuardaManipuladorQt(nl::foundation::log::Log* registro)
{
    instalarManipuladorQt(registro);
}

GuardaManipuladorQt::~GuardaManipuladorQt()
{
    instalarManipuladorQt(nullptr);
}

} // namespace nl::mid::composicao

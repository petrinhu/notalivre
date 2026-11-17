# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
"""Exceção de entrada ilegível do portão de funções no CTest, que sai com rc 2."""


class Parada(Exception):
    """Interrompe a varredura com rc 2 e uma mensagem."""

    def __init__(self, texto):
        super().__init__(texto)
        self.texto = texto

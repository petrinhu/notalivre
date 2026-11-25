-- SPDX-License-Identifier: GPL-3.0-or-later
-- SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
SELECT CONCAT(table_schema, '.', table_name) AS tabela FROM information_schema.tables WHERE table_schema IN ('notalivre_ra','notalivre_rb','notalivre_rc','notalivre_rd','notalivre_rz','outro_rn') ORDER BY 1;

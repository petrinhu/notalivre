-- SPDX-License-Identifier: GPL-3.0-or-later
-- SPDX-FileCopyrightText: 2026 Petrus Alves da Silva Costa
DROP DATABASE IF EXISTS notalivre_ra;
DROP DATABASE IF EXISTS notalivre_rb;
DROP DATABASE IF EXISTS notalivre_rc;
DROP DATABASE IF EXISTS notalivre_rd;
DROP DATABASE IF EXISTS notalivre_rz;
DROP DATABASE IF EXISTS outro_rn;
CREATE DATABASE notalivre_ra;
CREATE DATABASE notalivre_rb;
CREATE DATABASE notalivre_rc;
CREATE DATABASE notalivre_rd;
CREATE DATABASE outro_rn;
CREATE TABLE notalivre_ra.t1 (origem VARCHAR(32)) ENGINE=InnoDB;
INSERT INTO notalivre_ra.t1 VALUES ('ra.t1');
CREATE TABLE notalivre_ra.t2 (origem VARCHAR(32)) ENGINE=InnoDB;
INSERT INTO notalivre_ra.t2 VALUES ('ra.t2');
CREATE TABLE notalivre_ra.t3 (origem VARCHAR(32)) ENGINE=InnoDB;
INSERT INTO notalivre_ra.t3 VALUES ('ra.t3');
CREATE TABLE notalivre_rc.t1 (origem VARCHAR(32)) ENGINE=InnoDB;
INSERT INTO notalivre_rc.t1 VALUES ('rc.t1');
CREATE TABLE notalivre_rb.copia (origem VARCHAR(32)) ENGINE=InnoDB;
INSERT INTO notalivre_rb.copia VALUES ('rb.copia');

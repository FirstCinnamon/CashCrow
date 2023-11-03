PREPARE insert_account(text, char(32), char(256)) AS INSERT INTO account_security(email, salt, hash) VALUES($1, $2, $3);
PREPARE select_from_account_info AS SELECT * FROM account_info WHERE id = $1;
PREPARE select_from_account_security AS SELECT * FROM account_security WHERE email = $1;

PREPARE select_from_bank_account AS SELECT * FROM bank_account WHERE id = $1;
/*PREPARE insert_bank_account AS 
INSERT INTO bank_account(owner_id, bank_name, balance) VALUES($1, $2, $3);
미사용 Trigger로 해결 가능*/


--PREPARE select_from_trade_history AS SELECT * FROM trade_history WHERE id = $1;
--열람 쿼리 설계 필요
PREPARE insert_trade_history AS 
INSERT INTO trade_history (product, time_traded, price, buyer_id) VALUES($1, CURRENT_TIMESTAMP, $2, $3);

--PREPARE select_from_owned_stock AS 
--SELECT * FROM owned_stock WHERE owner_id = $1 AND name = $2;
--열람 쿼리 설계 필요
PREPARE upsert_owned_stock (int, varchar(20), int) AS
INSERT INTO owned_stock (owner_id, name, num)
VALUES ($1, $2, $3)
ON CONFLICT (owner_id, name)
DO UPDATE SET num = owned_stock.num + $3;
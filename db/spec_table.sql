CREATE TABLE account_info (
	id				SERIAL		not null primary key,
	account_balance	int			not null
);

CREATE TABLE account_security (
	id				SERIAL		not null primary key,
	email			text		not null,
	salt			char(32)	not null,
	hash			char(256)	not null
);

CREATE TABLE bank_account
(
	id				SERIAL		not null primary key,			
	owner_id		int			not null,
	bank_name		varchar(20)	not null,
	balance			int			not null, 
	FOREIGN KEY (owner_id) REFERENCES account_security(id)
);
--type can be altered to ‘money’ someday
CREATE TABLE trade_history (
	id				SERIAL		not null primary key,
	product			varchar(20)	not null,
	time_traded		TIMESTAMP	not null,
	price			int			not null,
	buyer_id		int			not null,
	FOREIGN KEY (buyer_id) REFERENCES account_security(id)
); 
/*type can be altered to ‘money’ someday*/
CREATE TABLE owned_stock (
	owner_id		int		not null,
	name			varchar(20)	not null,
	num				int			not null,
	PRIMARY KEY (owner_id, name),
	FOREIGN KEY (owner_id) REFERENCES account_security(id)
); 

CREATE OR REPLACE FUNCTION insert_account_info()
RETURNS TRIGGER AS $$
BEGIN
    -- account_info 테이블에 대응하는 레코드를 삽입합니다.
    INSERT INTO account_info (account_balance)
    VALUES (0); -- 필요에 따라 초기 잔액 값을 설정할 수 있습니다.

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- account_security 테이블에 트리거 생성
CREATE TRIGGER account_security_insert
AFTER INSERT ON account_security
FOR EACH ROW
EXECUTE FUNCTION insert_account_info();
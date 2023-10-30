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
--type can be altered to ��money�� someday
CREATE TABLE trade_history (
	id				SERIAL		not null primary key,
	product			varchar(20)	not null,
	time_traded		TIMESTAMP	not null,
	price			int			not null,
	buyer_id		int			not null,
	FOREIGN KEY (buyer_id) REFERENCES account_security(id)
); 
/*type can be altered to ��money�� someday*/
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
    -- account_info ���̺� �����ϴ� ���ڵ带 �����մϴ�.
    INSERT INTO account_info (account_balance)
    VALUES (0); -- �ʿ信 ���� �ʱ� �ܾ� ���� ������ �� �ֽ��ϴ�.

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- account_security ���̺� Ʈ���� ����
CREATE TRIGGER account_security_insert
AFTER INSERT ON account_security
FOR EACH ROW
EXECUTE FUNCTION insert_account_info();
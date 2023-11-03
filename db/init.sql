ALTER USER postgres PASSWORD '1234';

CREATE TABLE account_info (
 id SERIAL not null primary key,
 account_balance float4 not null
);

CREATE TABLE account_security (
 id    SERIAL  not null primary key,
 email   text  not null,
 salt   char(32) not null,
 hash   char(256) not null
);

CREATE TABLE bank_account
(
 id    SERIAL  not null primary key,   
 owner_id  int   not null,
 bank_name  varchar(20) not null,
 balance   float4  not null, 
 FOREIGN KEY (owner_id) REFERENCES account_security(id)
);

CREATE TABLE trade_history (
 id    SERIAL  not null primary key,
 product   varchar(20) not null,
 time_traded  TIMESTAMP not null,
 price   float4   not null,
 buyer_id  int   not null,
 FOREIGN KEY (buyer_id) REFERENCES account_security(id)
); 

CREATE TABLE owned_stock (
 owner_id  int   not null,
 name   varchar(20) not null,
 num    int   not null,
 PRIMARY KEY (owner_id, name),
 FOREIGN KEY (owner_id) REFERENCES account_security(id)
); 

PREPARE select_from_owned_stock(int) AS SELECT name, num FROM owned_stock WHERE owner_id = $1;

CREATE OR REPLACE FUNCTION insert_account_info()
RETURNS TRIGGER AS $$
BEGIN
    INSERT INTO account_info (account_balance)
    VALUES (0); 

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;


CREATE TRIGGER account_security_insert
AFTER INSERT ON account_security
FOR EACH ROW
EXECUTE FUNCTION insert_account_info();

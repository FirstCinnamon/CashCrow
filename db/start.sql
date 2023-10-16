CREATE TABLE owner (
	id				int			not null primary key,
	account_balance	int			not null 
);
--account_balance type can be altered to ¡®money¡¯ someday
CREATE TABLE bank_account
(
	id				int			not null primary key,			
	owner_id		int			not null,
	bank_name		varchar(20)	not null,
	balance			int			not null, 
	FOREIGN KEY (owner_id) REFERENCES owner(id)
);
--type can be altered to ¡®money¡¯ someday
CREATE TABLE trade_history (
	id				int			not null primary key,
	product			varchar(20)	not null,
	time_traded		DATE		not null,
	price			int			not null,
	buyer_id		int			not null,
	FOREIGN KEY (buyer_id) REFERENCES owner(id)
); 
/*type can be altered to ¡®money¡¯ someday*/
CREATE TABLE owned_stock (
	owner_id		int			not null,
	name			int			not null,
	num				varchar(20)	not null,
	PRIMARY KEY(owner_id, name),
	FOREIGN KEY (owner_id) REFERENCES owner(id)
); 

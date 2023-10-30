#include <pqxx/pqxx>
#include <iostream>

namespace db
{
    class DBConnection {
    public:
        pqxx::work* w;

        DBConnection(const std::string& host) {
            c = new pqxx::connection(host);
            if (c->is_open()) {
                std::cout << "Opened database successfully: " << c->dbname() << std::endl;
            }
            else {
                std::cerr << "Failed to open database" << std::endl;
                return;
            }
            w = new pqxx::work(*c);
            prepare();
        }

        int selectFromOwner(int id) {
            pqxx::params param(id);
            pqxx::result result = w->exec_prepared("select_from_owner", param);
            pqxx::row r = result.at(0);
            int ret = r[1].as<int>();
            w->commit();
            return ret;
        }

        std::tuple<int, std::string, int> selectFromBankAccount(int id) {
            pqxx::params param(id);
            pqxx::result result = w->exec_prepared("select_from_bank_account", param);
            pqxx::row r = result.at(0);
            std::tuple<int, std::string, int> ret;
            ret = std::make_tuple(r[1].as<int>(), r[2].as<std::string>(), r[3].as<int>());
            w->commit();
            return ret;
        }

        void insertOwner(int account_balance) {
            pqxx::params param(account_balance);
            pqxx::result result = w->exec_prepared("insert_owner", param);
            w->commit();
        }

        void insertBankAccount(int owner_id, const std::string& bank_name, int balance)
        {
            pqxx::params param(owner_id, bank_name, balance);
            pqxx::result result = w->exec_prepared("insert_bank_account", param);
            w->commit();
        }

        void insertTradeHistory(const std::string &product, int price, int buyer_id, int i) {
            //std::string sql = std::format("INSERT INTO bank_account (id, owner_id, bank_name, balance) SELECT COALESCE(MAX(id), 0) + 1, {}, '{}', {} FROM bank_account;",
            //    owner_id, bank_name, balance);
            //w->exec0(sql);
            //w->commit();
        }

        ~DBConnection() {
            if (w) w->commit();
            delete w;
            delete c;
        }

    private:
        pqxx::connection* c;

        void prepare() {
            w->exec("PREPARE insert_bank_account AS INSERT INTO bank_account(id, owner_id, bank_name, balance) SELECT COALESCE(MAX(id), 0) + 1, $1, $2, $3 FROM bank_account;");
            w->exec("PREPARE insert_owner AS INSERT INTO owner (id, account_balance) SELECT COALESCE(MAX(id), 0) + 1, $1 FROM owner;");
            //w->exec("PREPARE insert_trade_history AS INSERT INTO trade_history (id, product, time_traded, price, buyer_id) SELECT COALESCE(MAX(id), 0) + 1, $1, $2, $3, $4 FROM owner;");


            w->exec("PREPARE select_from_owner AS SELECT * FROM owner WHERE id = $1;");
            w->exec("PREPARE select_from_bank_account AS SELECT * FROM bank_account WHERE id = $1;");
        }
    };

}



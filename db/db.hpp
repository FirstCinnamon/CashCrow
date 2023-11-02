#pragma once
#include <pqxx/pqxx>
#include <iostream>

namespace db
{
    struct AccountPassword {
        std::string salt;
        std::string hash;
    };

    class DBConnection {
    public:
        pqxx::work* w;

        DBConnection(const std::string& host) {
            init(host);
        }
        DBConnection(const std::string& host, 
            const std::string& user,
            const std::string& dbname,
            const std::string& password) {
            auto formatted = string_format(
                "host=%s user=%s dbname=%s password=%s port=5432 connect_timeout=10",
                host.c_str(), user.c_str(), dbname.c_str(), password.c_str());
            init(formatted);
        }

#pragma region Login

        //email, salt, hash로 account_security에 insert합니다.
        //postgreSQL trigger function으로 'account_info'에도 자동으로 insert됩니다.
        void insertAccount(const std::string& email, const std::string& salt, const std::string& hash) {
            pqxx::params param(email, salt, hash);
            pqxx::result result = w->exec_prepared("insert_account", param);
            w->commit();
        }

        int selectIdFromAccountSecurity(const std::string& email) {
            pqxx::params param(email);
            pqxx::row r = w->exec_prepared1("select_from_account_security", email);
            int ret = r["id"].as<int>();
            return ret;
        }

        AccountPassword selectFromAccountSecurity(const std::string& email) {
            pqxx::params param(email);
            pqxx::row r = w->exec_prepared1("select_from_account_security", email);
            AccountPassword ret = { r["salt"].as<std::string>(), r["hash"].as<std::string>() };
            return ret;
        }

#pragma endregion
        int selectFromAccountInfo(int id) {
            pqxx::params param(id);
            pqxx::row r = w->exec_prepared1("select_from_account_info", param);
            int ret = r[1].as<int>();
            return ret;
        }

        std::map<std::string, int> selectFromOwnedStock(int ownerId) {
            pqxx::params param(ownerId);
            pqxx::result result = w->exec_prepared("select_from_owned_stock", param);

            std::map<std::string, int> ret;
            for (const auto& row : result) {
                std::string name = row["name"].as<std::string>();
                int num = row["num"].as<int>();
                ret[name] = num;
            }
            return ret;
        }

        //unmade
        std::tuple<int, std::string, int> selectFromBankAccount(int id) {
            pqxx::params param(id);
            pqxx::result result = w->exec_prepared("select_from_bank_account", param);
            pqxx::row r = result.at(0);
            std::tuple<int, std::string, int> ret;
            ret = std::make_tuple(r[1].as<int>(), r[2].as<std::string>(), r[3].as<int>());
            w->commit();
            return ret;
        }

        void insertBankAccount(int owner_id, const std::string& bank_name, int balance)
        {
            pqxx::params param(owner_id, bank_name, balance);
            pqxx::result result = w->exec_prepared("insert_bank_account", param);
            w->commit();
        }

        void insertTradeHistory(const std::string& product, int price, int buyer_id) {
            pqxx::params param(product, price, buyer_id);
            pqxx::result result = w->exec_prepared("insert_trade_history", param);
            w->commit();
        }

        void upsertOwnedStock(int owner_id, const std::string& name, int num) {
            pqxx::params param(owner_id, name, num);
            pqxx::result result = w->exec_prepared("upsert_owned_stock", param);
            w->commit();
        }

        ~DBConnection() {
            if (w) w->commit();
            delete w;
            delete c;
        }

    private:
        pqxx::connection* c;

        void init(const std::string& options)
        {
            c = new pqxx::connection(options);
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

        template<typename ... Args>
        std::string string_format(const std::string& format, Args ... args)
        {
            size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
            if (size <= 0) { throw std::runtime_error("Error during formatting."); }
            std::unique_ptr<char[]> buf(new char[size]);
            snprintf(buf.get(), size, format.c_str(), args ...);
            return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
        }

        void prepare() {
            //account
            w->exec("PREPARE insert_account(text, char(32), char(256)) AS INSERT INTO account_security(email, salt, hash) VALUES($1, $2, $3);");
            w->exec("PREPARE select_from_account_info AS SELECT * FROM account_info WHERE id = $1;");
            w->exec("PREPARE select_from_account_security AS SELECT * FROM account_security WHERE email = $1;");

            //bank_account
            w->exec("PREPARE select_from_bank_account AS SELECT * FROM bank_account WHERE id = $1;");
            /*w->exec(R"(
PREPARE insert_bank_account AS 
INSERT INTO bank_account(owner_id, bank_name, balance) VALUES($1, $2, $3);
)");*/

            //trade_history
            w->exec("PREPARE select_from_trade_history AS SELECT * FROM trade_history WHERE id = $1;");
            w->exec(R"(
PREPARE insert_trade_history AS 
INSERT INTO trade_history (product, time_traded, price, buyer_id) VALUES($1, CURRENT_TIMESTAMP, $2, $3);
)");

            //owned_stock
            w->exec(R"(
PREPARE select_from_owned_stock(int) AS 
SELECT name, num FROM owned_stock WHERE owner_id = $1;
)");
            w->exec(R"(
PREPARE upsert_owned_stock (int, varchar(20), int) AS
INSERT INTO owned_stock (owner_id, name, num)
VALUES ($1, $2, $3)
ON CONFLICT (owner_id, name)
DO UPDATE SET num = owned_stock.num + $3;
)");
        }
    };

}
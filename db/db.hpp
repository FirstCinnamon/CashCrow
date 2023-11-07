#pragma once
#include <pqxx/pqxx>
#include <iostream>
#include <thread>
#include <mutex>
#include <future>

namespace db
{
    struct AccountPassword {
        std::string salt;
        std::string hash;
    };

    struct BankAccount {
        int id;
        std::string bank_name;
        float balance;
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

        //username, salt, hash�� account_security�� insert�մϴ�.
        //postgreSQL trigger function���� 'account_info'���� �ڵ����� insert�˴ϴ�.
        void insertAccount(const std::string& username, const std::string& salt, const std::string& hash) {
            pqxx::params param(username, salt, hash);
            pqxx::result result = w->exec_prepared("insert_account", param);
            w->commit();
        }

        int selectIdFromAccountSecurity(const std::string& username) {
            pqxx::params param(username);
            pqxx::row r = w->exec_prepared1("select_from_account_security", username);
            int ret = r["id"].as<int>();
            return ret;
        }

        AccountPassword selectFromAccountSecurity(const std::string& username) {
            pqxx::params param(username);
            pqxx::row r = w->exec_prepared1("select_from_account_security", username);
            AccountPassword ret = { r["salt"].as<std::string>(), r["hash"].as<std::string>() };
            return ret;
        }

        void tryInsertSession(int uid) {
            pqxx::params param(uid);
            pqxx::row row = w->exec_prepared1("exist_session_already", param);

            if (!row[0].as<bool>()) {
                pqxx::params param(uid);
                w->exec_prepared("insert_session", param);
                w->commit();
            }
        }

        void deleteSession(int uid) {
            pqxx::params param(uid);
            w->exec_prepared("delete_session", param);
            w->commit();
        }

        int sidToUid(int sid) {
            pqxx::params param(sid);
            pqxx::row row = w->exec_prepared1("sid_to_uid", param);
            int ret = row[0].as<int>();
            w->commit();
            return ret;
        }

        int uidToSid(const int& uid)
        {
            pqxx::params param(uid);
            pqxx::row row{w->exec_prepared1("uid_to_sid", param)};
            return row[0].as<int>();
        }

        bool isValidSid(const std::string& sid)
        {
            pqxx::params param(sid);
            pqxx::result result{w->exec_prepared("is_valid_sid", param)};
            return result.size() == 1;
        }

#pragma endregion
        float selectFromAccountInfo(int id) {
            pqxx::params param(id);
            pqxx::row r = w->exec_prepared1("select_from_account_info", param);
            float ret = r[1].as<float>();
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

        std::map<std::string, float> selectFromAvgPrice(int ownerId) {
            pqxx::params param(ownerId);
            pqxx::result result = w->exec_prepared("select_from_avg_price", param);

            std::map<std::string, float> ret;
            for (const auto& row : result) {
                std::string name = row["product"].as<std::string>();
                int num = row["price_avg"].as<float>();
                ret[name] = num;
            }
            return ret;
        }

        std::vector<BankAccount> selectFromBankAccount(int id) {
            pqxx::params param(id);
            pqxx::result result = w->exec_prepared("select_from_bank_account", param);
            std::vector<BankAccount> bankAccounts;
            for (pqxx::result::const_iterator row = result.begin(); row != result.end(); ++row) {
                BankAccount account;
                account.id = row["id"].as<int>();
                account.bank_name = row["bank_name"].as<std::string>();
                account.balance = row["balance"].as<float>();

                bankAccounts.push_back(account);
            }
            w->commit();
            return bankAccounts;
        }

        //���̰� �ʹٸ� balance�� ���� �Է�
        void increaseBankAccount(int id, float balance) {
            pqxx::params param(id, balance);
            pqxx::result result = w->exec_prepared("increase_bank_account", param);
            w->commit();
        }

        void insertBankAccount(int owner_id, const std::string& bank_name, float balance)
        {
            pqxx::params param(owner_id, bank_name, balance);
            pqxx::result result = w->exec_prepared("insert_bank_account", param);
            w->commit();
        }

        void insertTradeHistory(const std::string& product, float price, int buyer_id) {
            pqxx::params param(product, price, buyer_id);
            pqxx::result result = w->exec_prepared("insert_trade_history", param);
            w->commit();
        }

        void upsertOwnedStock(int owner_id, const std::string& name, int num) {
            pqxx::params param(owner_id, name, num);
            pqxx::result result = w->exec_prepared("upsert_owned_stock", param);
            w->commit();
        }

        void upsertAvgPrice(int owner_id, const std::string& name, float avg_price) {
            pqxx::params param(owner_id, name, avg_price);
            pqxx::result result = w->exec_prepared("upsert_avg_price", param);
            w->commit();
        }

        void changeAccount(int owner_id, float num) {
            pqxx::params param(owner_id, num);
            pqxx::result result = w->exec_prepared("increase_account", param);
            w->commit();
        }

        void createNewTransObj()
        {
            delete w;
            w = new pqxx::work(*c);
        }


        /*void reduceOwnedStock(int owner_id, const std::string& name, int num) {
            pqxx::params param(owner_id, name, num);
            pqxx::result result = w->exec_prepared("reduce_owned_stock", param);
            w->commit();
        }*/

        ~DBConnection() {
            a.store(true);
            if (w) w->commit();
            delete w;
            delete c;
        }
        typedef void (*FunctionPointer)(void);
    private:
        pqxx::connection* c;
        std::atomic_bool a;

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

            // DBConnection* self = this;
            // a.store(false);
            // setInterval(a, static_cast<size_t>(1000) , [this]() {deleteIfExpiryOver(); });
        }

        void deleteIfExpiryOver() {
            w->exec0("DELETE FROM session WHERE expiry < NOW(); ");
            w->commit();
        }

        template <class F, class... Args>
        void setInterval(std::atomic_bool& cancelToken, size_t interval, F&& f, Args&&... args) {
            cancelToken.store(true);
            auto cb = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            std::async(std::launch::async, [=, &cancelToken]()mutable {
                while (cancelToken.load()) {
                    cb();
                    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                }
                });
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
            w->exec("PREPARE insert_account(varchar(20), char(20), char(64)) AS INSERT INTO account_security(username, salt, hash) VALUES($1, $2, $3);");
            w->exec("PREPARE select_from_account_info AS SELECT * FROM account_info WHERE id = $1;");

            w->exec(R"(
PREPARE increase_account (int, float) AS
UPDATE account_info SET account_balance = account_balance + $2
WHERE id = $1;
)");

            w->exec("PREPARE select_from_account_security AS SELECT * FROM account_security WHERE username = $1;");

            w->exec("PREPARE insert_session AS INSERT INTO session(uid) VALUES($1);");
            w->exec(R"(
PREPARE exist_session_already AS
SELECT COUNT(*) > 0
FROM session
WHERE uid = $1;
)");

            w->exec("PREPARE uid_to_sid AS SELECT sid FROM session WHERE uid = $1;");

            w->exec("PREPARE is_valid_sid AS SELECT * FROM session WHERE sid = $1;");

            w->exec(R"(
PREPARE delete_session AS DELETE FROM session WHERE uid = $1
)");
            w->exec("PREPARE sid_to_uid AS SELECT uid FROM session WHERE sid = $1;");

            //bank_account
            w->exec("PREPARE select_from_bank_account AS SELECT * FROM bank_account WHERE owner_id = $1;");
            w->exec(R"(PREPARE increase_bank_account(int, float) AS
UPDATE bank_account SET balance = balance + $2
WHERE id = $1)");
            w->exec(R"(
PREPARE insert_bank_account AS
INSERT INTO bank_account(owner_id, bank_name, balance) VALUES($1, $2, $3);
)");

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

            // avg price
            w->exec(R"(
PREPARE select_from_avg_price(int) AS
SELECT product, price_avg FROM avg_price WHERE buyer_id = $1;
)");

            w->exec(R"(
PREPARE upsert_avg_price (int, varchar(20), float) AS
INSERT INTO avg_price (product, price_avg, buyer_id)
VALUES ($2, $3, $1)
ON CONFLICT (product, buyer_id)
DO UPDATE SET price_avg = $3;
)");

            w->exec(R"(
PREPARE reduce_owned_stock(int, varchar, int) AS
UPDATE owned_stock
SET num = num - $3
WHERE owner_id = $1 AND name = $2;
)");
        }
    };

}


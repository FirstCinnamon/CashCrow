#include <pqxx/pqxx>
#include <iostream>

namespace db
{
    class DBConnection {
    public:
        pqxx::work* w;

        DBConnection(const std::string& host) {
            c = new pqxx::connection(host);
            w = new pqxx::work(*c);
        }

        void insertOwner(int account_balance) {
            std::string sql = std::format("INSERT INTO owner (id, account_balance) SELECT COALESCE(MAX(id), 0) + 1, {} FROM owner; ", account_balance);
            //pqxx::row r =w->exec1("SELECT current_database();");
            w->exec0(sql);
            w->commit();
        }

        void insertBankAccount( int owner_id, const std::string& bank_name, int balance)
        {
            std::string sql = std::format("INSERT INTO bank_account (id, owner_id, bank_name, balance) SELECT COALESCE(MAX(id), 0) + 1, {}, '{}', {} FROM bank_account;", 
                owner_id, bank_name, balance);
            w->exec0(sql);
            w->commit();
        }

        void insertTradeHistory(const std::string& product, int price, int buyer_id) {
            //std::string sql = std::format("INSERT INTO bank_account (id, owner_id, bank_name, balance) SELECT COALESCE(MAX(id), 0) + 1, {}, '{}', {} FROM bank_account;",
            //    owner_id, bank_name, balance);
            //w->exec0(sql);
            //w->commit();
        }

        void doSomething() {
            // w를 사용하여 작업 수행
            w->exec("INSERT INTO mytable VALUES (1, 'data')");
        }

        ~DBConnection() {
            if (w) w->commit();
            delete w;
            delete c;
        }

    private:
        pqxx::connection* c;
    };

    class Fraction
    {
    private:
        int m_numerator;   // 분자
        int m_denominator; // 분모

    public:
        Fraction() // default constructor
        {
            m_numerator = 0;
            m_denominator = 1;
        }

        int getNumerator() { return m_numerator; }
        int getDenominator() { return m_denominator; }
        double getValue() { return static_cast<double>(m_numerator) / m_denominator; }
    };

}

int main() {
    db::DBConnection con("host=localhost user=postgres dbname=postgres password=PASSWORD port=5432 connect_timeout=10");

    con.insertBankAccount(1, "abc", 400);
    //pqxx::row r = con.w->exec1("SELECT name from game limit 1");

    //con.w->commit();

    //std::cout << r[0].as<std::string>() << std::endl;
    return 0;
}


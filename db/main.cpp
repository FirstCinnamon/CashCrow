#include <pqxx/pqxx>
#include <iostream>

namespace db
{
    class MyDatabaseConnection {
    public:
        std::unique_ptr<pqxx::work> w;

        MyDatabaseConnection(const std::string& host) {
            c = std::make_unique<pqxx::connection>(host);
            w = std::make_unique<pqxx::work>(*c);
        }

        void insertOwner(int id, int account_balance) {

        }

        void insertBankAccount(int id, int owner_id, std::string& bank_name, int balance)
        {

        }

        void insertTradeHistory() {

        }

        void doSomething() {
            // w를 사용하여 작업 수행
            w->exec("INSERT INTO mytable VALUES (1, 'data')");
        }

        ~MyDatabaseConnection() {
            // 소멸자에서 연결을 안전하게 닫음
            if (w) w->commit();
        }

    private:
        std::unique_ptr<pqxx::connection> c;
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
    db::MyDatabaseConnection con("host=localhost user=postgres dbname=students password=PASSWORD port=5432 dbname=students connect_timeout=10");
    /*pqxx::connection c("host=localhost user=postgres dbname=students password=PASSWORD port=5432 dbname=students connect_timeout=10");

    pqxx::work w(c);*/

    pqxx::row r = con.w->exec1("SELECT name from game limit 1");

    con.w->commit();

    std::cout << r[0].as<std::string>() << std::endl;
    return 0;
}


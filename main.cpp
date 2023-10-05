#include "crow.h"

int main()
{
    // Define the crow application
    crow::SimpleApp app;

    // Define the endpoint at the root directory
    CROW_ROUTE(app, "/")([](){
        return "CashCrow";
    });

    // Set the port, set the app to run on multiple threads, and run the app
    app.port(18080).multithreaded().run();
}

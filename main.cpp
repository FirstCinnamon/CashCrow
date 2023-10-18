#include "crow.h"
#include "crow/app.h"
#include "crow/common.h"
#include "crow/http_response.h"
#include "crow/mustache.h"
#include "crow/middlewares/cors.h"
#include "crow/query_string.h"
#include "crow/middlewares/session.h"
#include <cstring>
#include <ctime>
#include <string>

// redirect to root: if logined then dashboard, else login page
crow::response redirect()
{
    crow::response res;
    res.redirect("/");
    return res;
}

bool is_sid_valid(std::string &sid)
{
    // check out DB so that given sid is matched with a user
    // for test purpose, only sid=1234 is valid
    if (strcmp("1234", sid.data()) == 0) { // change this condition later
    return true;
  } else {
    return false;
  }
}

int main()
{
  // Global Template directory
  crow::mustache::set_global_base("html");

  // Define the crow application with session and CORSHandler middleware
  // InMemoryStore stores all entries in memory
  using Session = crow::SessionMiddleware<crow::InMemoryStore>;
  crow::App<crow::CookieParser, Session, crow::CORSHandler> app{Session{
      // customize cookies
      crow::CookieParser::Cookie("session").max_age(/*one day*/ 24 * 60 * 60).path("/"),
      // set session id length (small value only for demonstration purposes)
      4,
      // init the store
      crow::InMemoryStore{}
  }};

  // Define the endpoint at the root directory
  CROW_ROUTE(app, "/")
  ([&](const crow::request& req)
   {
      crow::response response("");
      
      // get session as middleware context
      auto& session = app.get_context<Session>(req);

      // get session id(sid) which is temporarily an integer
      // return empty string if value is now found
      std::string string_v = session.get<std::string>("sid");
      if (string_v.empty()) {
        // if sid is not set(not yet login-ed), so show login page
        // for test purpose, root page is login page
        auto page = crow::mustache::load("login.html");
        response.write(page.render_string());
        return response;
      } else {
        // session id exists, so go check if sid is valid and show user's dashboard
        if (is_sid_valid(string_v)) {
          // valid sid, so load user's dashboard page
          response.redirect("/dashboard");
          return response;        
        } else {
          // invalid sid, so remove sid and redirect to root directory
          session.remove("sid");
          return redirect();
        }
      }});

  // for test purpose, login is only done if uid=test and pw=test
  CROW_ROUTE(app, "/login")
      .methods(crow::HTTPMethod::POST)([&](
        const crow::request &req)
       {
      // get session as middleware context
        auto& session = app.get_context<Session>(req);
        const crow::query_string ret = req.get_body_params();
        crow::response response("");

        if (strcmp(ret.get("uname"), "test") == 0 && strcmp(ret.get("pass"), "test") == 0)
        {
           // success, so give user an sid
           // sid generation should be randomized
           session.set("sid", "1234");
           // redirect to root page
           response.add_header("HX-Redirect", "http://localhost:18080/");
           return response;
         }
         // login failed, show retry message with uname field filled
         crow::mustache::context my_context;
         my_context["uname"] = ret.get("uname");
         auto page = crow::mustache::load("login_failure.html");
         response.write(page.render_string(my_context));
         return response;
       });

  CROW_ROUTE(app, "/logout")
      .methods(crow::HTTPMethod::POST)([&](
        const crow::request &req)
   {
        crow::response response("");
        auto& session = app.get_context<Session>(req);
        session.remove("sid");
        response.add_header("HX-Redirect", "http://localhost:18080/");
        return response;
   });

  CROW_ROUTE(app, "/dashboard")
  ([&](const crow::request& req)
   {
      crow::response response("");

      // get session as middleware context
      auto& session = app.get_context<Session>(req);
      std::string string_v = session.get<std::string>("sid");

      // go check if sid is valid and show user's dashboard
      if (is_sid_valid(string_v) && !string_v.empty()) {
        // valid sid, so user's contents
        // needs more work to show customized page
        auto page = crow::mustache::load("index.html");
        response.write(page.render_string());
        return response;
      } else {
        // invalid sid, so remove sid and redirect to root directory
        session.remove("sid");
        return redirect();
      }
  });

  CROW_ROUTE(app, "/goto_login")
  ([]()
   {
      crow::response response("");
      response.add_header("HX-Redirect", "http://localhost:18080/");
      return response; });

  CROW_ROUTE(app, "/goto_register")
  ([]()
   {
     auto page = crow::mustache::load("register.html");
     return page.render();
     // modulating html required
   });

  // for test purpose, register is only done if uid=test and pw=test
  CROW_ROUTE(app, "/register")
      .methods(crow::HTTPMethod::POST)([](
         const crow::request &req)
     {
       const crow::query_string ret = req.get_body_params();
       crow::response response("");

       if (strcmp(ret.get("username"), "test") == 0 && strcmp(ret.get("password"), "test") == 0)
       {
         // success and show welcom message with link to login page
         crow::mustache::context my_context;
         my_context["username"] = ret.get("username");
         auto page = crow::mustache::load("register_success.html");
         response.write(page.render_string(my_context));
         return response;
       }

       // validation failed: eg) password not long enough, username duplication etc
       // error message
       crow::mustache::context my_context;
       my_context["error_message"] = "THIS IS USEFUL ERROR MESSAGE";
       auto page = crow::mustache::load("register_failure.html");
       response.write(page.render_string(my_context));
       return response;
     });

  // Set the port, set the app to run on multiple threads, and run the app
  app.port(18080).multithreaded().run();
}

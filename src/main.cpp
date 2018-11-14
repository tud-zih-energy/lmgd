#include <lmgd/source/source.hpp>

#include <lmgd/log.hpp>

#include <nitro/broken_options/parser.hpp>
#include <nitro/lang/enumerate.hpp>

#include <iostream>
#include <stdexcept>

using lmgd::Log;

// TODO setup signal handler and clean up properly ...

int main(int argc, char* argv[])
{
    nitro::broken_options::parser parser("lmgd");

    parser.option("server", "The metricq management server to connect to.")
        .default_value("amqp://localhost")
        .short_name("s");
    parser.option("token",
                  "The token used for source authentification against the metricq manager.");
    parser.toggle("help").short_name("h");
    parser.toggle("debug").short_name("d");
    parser.toggle("trace").short_name("t");
    parser.toggle("drop-data").short_name("x");

    try
    {
        auto options = parser.parse(argc, argv);

        if (!options.given("trace"))
        {
            if (!options.given("debug"))
            {
                lmgd::set_severity_info();
            }
            else
            {
                lmgd::set_severity_debug();
            }
        }

        if (options.given("help"))
        {
            parser.usage();

            return 0;
        }

        lmgd::initialize_logger();

        lmgd::source::Source source(options.get("server"), options.get("token"),
                                    options.given("drop-data"));

        source.main_loop();
    }
    catch (nitro::broken_options::parser_error& e)
    {
        lmgd::Log::fatal() << e.what();

        parser.usage();

        return 1;
    }
    catch (std::exception& e)
    {
        lmgd::Log::fatal() << e.what();
    }

    return 0;
}

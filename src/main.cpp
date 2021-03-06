#include <lmgd/source/source.hpp>

#include <lmgd/log.hpp>

#include <nitro/options/parser.hpp>
#include <nitro/lang/enumerate.hpp>

#include <iostream>
#include <stdexcept>

using lmgd::Log;

// TODO setup signal handler and clean up properly ...

int main(int argc, char* argv[])
{
    nitro::options::parser parser("lmgd");

    parser.option("server", "The metricq management server to connect to.")
        .default_value("amqp://localhost")
        .short_name("s");
    parser.option(
        "token", "The token used for source authentification against the metricq manager.");
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
                metricq::logger::nitro::set_severity(nitro::log::severity_level::info);
            }
            else
            {
                metricq::logger::nitro::set_severity(nitro::log::severity_level::debug);
            }
        }

        if (options.given("help"))
        {
            parser.usage();

            return 0;
        }

        metricq::logger::nitro::initialize();

        lmgd::source::Source source(
            options.get("server"), options.get("token"), options.given("drop-data"));

        source.main_loop();
    }
    catch (nitro::options::parsing_error& e)
    {
        std::cerr << e.what() << '\n';

        parser.usage();

        return 1;
    }
    catch (std::exception& e)
    {
        lmgd::Log::fatal() << e.what();
        return 2;
    }

    return 0;
}

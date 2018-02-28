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
    lmgd::set_severity_debug();

    nitro::broken_options::parser parser("lmgd");

    parser.option("server", "The dataheap2 management server to connect to.")
        .default_value("amqp://localhost")
        .short_name("s");
    parser.option("token",
                  "The token used for source authentification against the dataheap2 manager.");
    parser.toggle("help").short_name("h");

    try
    {
        auto options = parser.parse(argc, argv);

        if (options.given("help"))
        {
            parser.usage();

            return 0;
        }

        lmgd::source::Source source(options.get("server"), options.get("token"));

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

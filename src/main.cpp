#include <lmgd/source/source.hpp>

#include <lmgd/log.hpp>

#include <nitro/broken_options/parser.hpp>
#include <nitro/lang/enumerate.hpp>

#include <iostream>
#include <stdexcept>

using lmgd::Log;

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
//
// void foo()
// {
//
//     lmgd::set_severity_debug();
//
//     lmgd::Log::info() << "HELO!";
//
//     nitro::broken_options::parser parser("lmgd");
//
//     parser.option("hostname").default_value("localhost");
//     parser.option("config").short_name("c");
//
//     parser.toggle("help").short_name("h");
//
//     try
//     {
//         auto options = parser.parse(argc, argv);
//
//         if (options.given("help"))
//         {
//             parser.usage();
//
//             return 0;
//         }
//
//         lmgd::config::ConfigReader config(options.get("config"));
//
//         std::string hostname = options.get("hostname");
//
//         lmgd::network::Connection socket(hostname);
//
//         Log::info() << "Connected.";
//
//         socket.send_command("ERRALL?");
//         Log::info() << "errors before: " << socket.read_ascii();
//
//         socket.send_command("LPFILT?");
//         Log::info() << "LPCOFRNG: " << socket.read_ascii();
//
//         socket.send_command("GROUP 3,3");
//
//         socket.send_command("IAUTO1 0; UAUTO1 0; IRNG1 32; URNG1 12.5");
//         socket.send_command("IAUTO2 0; UAUTO2 0; IRNG2 16; URNG2 12.5");
//
//         socket.send_command("IAUTO3 0; UAUTO3 0; IRNG3 32; URNG3 12.5");
//         // socket.send_command("IAUTO4 0; UAUTO4 0; IRNG4 16; URNG4 12.5");
//
//         // socket.send_command("IAUTO5 0; UAUTO5 0; IRNG5 8; URNG5 12.5");
//         // socket.send_command("IAUTO6 0; UAUTO6 0; IRNG6 8; URNG6 12.5");
//
//         socket.send_command("GLCTRAC 0, \"P1111\"");
//         socket.send_command("GLCTRAC 1, \"P1211\"");
//         socket.send_command("GLCTRAC 2, \"P1311\"");
//         socket.send_command("GLCTRAC 3, \"P1411\"");
//         socket.send_command("GLCTRAC 4, \"P1511\"");
//         socket.send_command("GLCTRAC 5, \"P1611\"");
//
//         socket.send_command("GLCSR 500000000");
//
//         socket.send_command("CYCLMOD SCOPE");
//         socket.send_command("INIM");
//
//         socket.send_command("GLCSR?");
//         Log::info() << "Requested Rate: " << socket.read_ascii();
//
//         socket.send_command("GLPSR?");
//         Log::info() << "Rate: " << socket.read_ascii();
//
//         socket.send_command("GLPTLEN?");
//         auto gap_length = std::stoi(socket.read_ascii());
//         Log::info() << "Gap length: " << gap_length;
//
//         socket.send_command("ACTN; TSSP?; GLPVAL? 0, (0:" + std::to_string(gap_length - 1) +
//                             "); GLPVAL? 1, (0:" + std::to_string(gap_length - 1) +
//                             "); GLPVAL? 2, (0:" + std::to_string(gap_length - 1) + ")");
//
//         socket.send_command("ERRALL?");
//         Log::info() << "errors: " << socket.read_ascii();
//
//         socket.mode(lmgd::network::Connection::Mode::binary);
//
//         socket.send_command("CONT ON");
//
//         for (int i = 0; i < 10; i++)
//         {
//             auto dataline = socket.read_binary();
//
//             auto timestamp = dataline.read_date();
//
//             Log::info() << "Timestamp: " << timestamp.time_since_epoch().count();
//
//             for (int i = 0; i < 3; i++)
//             {
//                 auto list = dataline.read_float_list();
//                 auto line = Log::info() << "Track " << i << ": " << list.size() << " values: ";
//                 for (auto value : nitro::lang::enumerate(list))
//                 {
//                     if (value.index() > 10)
//                         break;
//                     line << value.value() << ", ";
//                 }
//             }
//             Log::info() << "DataBuffer position: " << dataline.position() << " / "
//                         << dataline.size();
//         }
//
//         socket.send_command("CONT OFF");
//
//         socket.mode(lmgd::network::Connection::Mode::ascii);
//
//         socket.send_command("ERRALL?");
//         Log::info() << "errors: " << socket.read_ascii();
//     }
//     catch (std::exception& e)
//     {
//         lmgd::Log::fatal() << e.what();
//     }
//
//     return 0;
// }

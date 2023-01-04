#include "tools.hpp"
#include <xpar.hpp>

//#define XML_PRINT

namespace xpar_testing
{
    struct xpar_custom_config
    {
        using char_t = char;

        enum const_t
        {
            try_continue_on_error = 0,
            max_name_length = 32,
            max_value_length = 64,
            max_stack_size = 16
        };
    };

    template <typename _Config = xpar_custom_config>
    class counting_observer: public stdext::counting_observer
    {
    public:
        using xpar_t = stdext::xpar<counting_observer, _Config>;
        using char_t = typename xpar_t::char_t;
        using base_t = stdext::counting_observer;
        using base_t::attribute_count;
        using base_t::comment_count;
        using base_t::data_count;
        using base_t::element_count;
        using base_t::error_count;

        std::string buffer {};

        void on_element_begin(xpar_t& /*parser*/, const char_t* name, const char_t* name_end)
        {
#ifdef XML_PRINT
            buffer.assign(name, name_end);
            std::cout << "> " << buffer << '\n';
#else
            static_cast<void>(name);
            static_cast<void>(name_end);
#endif
        }

        void on_element_end(xpar_t& /*parser*/, const char_t* /*name*/, const char_t* /*name_end*/)
        {
            ++element_count;
        }
        void on_attribute(xpar_t& /*parser*/, const char_t* /*name*/, const char_t* /*name_end*/)
        {
            ++attribute_count;
        }
        void on_attribute_value(xpar_t& /*parser*/, const char_t* /*text*/, const char_t* /*text_end*/, const bool /*partial*/)
        {
        }
        void on_data(xpar_t& /*parser*/, const char_t* text, const char_t* text_end, const bool /*partial*/)
        {
#ifdef XML_PRINT
            buffer.assign(text, text_end);
            std::cout << "> " << buffer << '\n';
#else
            static_cast<void>(text);
            static_cast<void>(text_end);
#endif
            ++data_count;
        }
        void on_comment(xpar_t& /*parser*/, const char_t* /*text*/, const char_t* /*text_end*/, const bool /*partial*/)
        {
            ++comment_count;
        }
        void on_error(xpar_t& parser, bool& /*try_continue*/)
        {
            std::cout << parser.line() << ':' << parser.column() << " error" << std::endl;
            ++error_count;
        }
    };

    class test: public stdext::test<counting_observer<>>
    {
    public:
        using base_t = stdext::test<counting_observer<>>;
        using base_t::observer;
        using base_t::xml_data_t;

        test(const char* data_path): base_t("xpar", data_path) {}

    private:
        void execute(const xml_data_t& xml_data) override
        {
            counting_observer<>::xpar_t parser(&observer());
            parser(xml_data.data(), xml_data.size());
        }
    };
}

int main(const int /*argc*/, const char* const argv[])
{
    xpar_testing::test test(argv[1U]);
    test.run();
    return 0;
}

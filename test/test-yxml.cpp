#include "tools.hpp"
#include <yxml.h>

//#define XML_PRINT

namespace yxml_testing
{
    class test: public stdext::test<>
    {
    public:
        test(const char* data_path): stdext::test<>("yxml", data_path) {}

    private:
        std::vector<char> stack_ = std::vector<char>(4096U);

        void execute(const xml_data_t& xml_data) override
        {
            yxml_t parser_data {};
            yxml_init(&parser_data, stack_.data(), stack_.size());
            yxml_ret_t result {};
            bool data {};
            for (auto c: xml_data)
            {
                result = yxml_parse(&parser_data, c);
                switch (result)
                {
                    case YXML_OK:
                        break;
                    case YXML_ELEMSTART:
                        ++observer().element_count;
                        break;
                    case YXML_CONTENT:
                        if (!data)
                        {
                            data = true;
                            ++observer().data_count;
                        }
                        break;
                    case YXML_ATTRSTART:
                        ++observer().attribute_count;
                    case YXML_ELEMEND:
                    case YXML_ATTREND:
                        if (data)
                        {
                            data = false;
                        }
                        break;
                    default:
                        if (data)
                        {
                            data = false;
                        }

                        if (result < 0)
                        {
                            ++observer().error_count;
                            return;
                        }
                        break;
                }
            }

            if (result >= 0)
            {
                static_cast<void>(yxml_eof(&parser_data));
            }
        }
    };
}

int main(const int /*argc*/, const char* const argv[])
{
    yxml_testing::test test(argv[1U]);
    test.run();
    return 0;
}

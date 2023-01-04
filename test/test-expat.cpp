#include "tools.hpp"
#include <expat.h>

//#define XML_PRINT

namespace expat_testing
{
    using observer_t = stdext::test<>::observer_t;
    observer_t* observer {};

    void on_element_begin(void* /*userData*/, const char* name, const char* attributes[])
    {
#ifdef XML_PRINT
        std::cout << "> " << name << '\n';
        std::cout << name << ": ";
#else
        static_cast<void>(name);
        static_cast<void>(attributes);
#endif
        ++observer->element_count;
        for (auto i = 0U; attributes[i] != nullptr; i += 2U)
        {
            ++observer->attribute_count;
#ifdef XML_PRINT
            std::cout << attributes[i] << '=' << attributes[i + 1U] << '\n';
#endif
        }
    }

    void on_data(void* /*userData*/, const char* val, int len)
    {
        if (isalnum(*val))
        {
            ++observer->data_count;
#ifdef XML_PRINT
            static std::string buffer {};
            buffer.assign(val, val + len);
            std::cout << "> " << buffer << '\n';
#else
            static_cast<void>(val);
            static_cast<void>(len);
#endif
        }
    }

    void on_comment(void* /*userData*/, const XML_Char* /*data*/)
    {
        ++observer->comment_count;
    }

    class test: public stdext::test<>
    {
    public:
        test(const char* data_path): stdext::test<>("expat", data_path) {}

    private:
        void execute(const xml_data_t& xml_data) override
        {
            auto parser = XML_ParserCreate(nullptr);
            XML_SetElementHandler(parser, on_element_begin, nullptr);
            XML_SetCharacterDataHandler(parser, on_data);
            XML_SetCommentHandler(parser, on_comment);

            XML_Parse(parser, xml_data.data(), xml_data.size(), XML_TRUE);
            XML_ParserFree(parser);
        }
    };
}

int main(const int /*argc*/, const char* const argv[])
{
    using namespace expat_testing;
    test test(argv[1U]);
    observer = &test.observer();
    test.run();
    observer = {};
    return 0;
}

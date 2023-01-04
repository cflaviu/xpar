#pragma once
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace stdext
{
    template <typename _Char = char>
    std::vector<_Char> read_file(const std::string& file_name)
    {
        using namespace std;
        ifstream file(file_name.data(), ios::in | ios::binary | ios::ate);
        const auto file_size = file.tellg();
        file.seekg(0, ios::beg);
        std::vector<_Char> bytes(file_size);
        file.read(reinterpret_cast<char*>(bytes.data()), file_size);
        return bytes;
    }

    inline std::vector<std::string> xml_files(const char* data_folder_path)
    {
        std::string path = data_folder_path;
        if (path.back() != '/')
        {
            path += '/';
        }

        return {
            path + "sample.xml",      //
            path + "nes96.xml",       //
            path + "ns_att_test.xml", //
            path + "recset.xml",      //
            // path + "wordnet_glossary-20010201.rdf", //
        };
    }

    struct counting_observer
    {
        std::size_t element_count {};
        std::size_t attribute_count {};
        std::size_t data_count {};
        std::size_t comment_count {};
        std::size_t error_count {};

        void clear() noexcept
        {
            element_count = {};
            attribute_count = {};
            data_count = {};
            comment_count = {};
            error_count = {};
        }
    };

    inline std::ostream& operator<<(std::ostream& out, const counting_observer& observer)
    {
        using namespace std;
        out << "element_count  : " << observer.element_count << '\n';
        out << "attribute count: " << observer.attribute_count << '\n';
        out << "data count     : " << observer.data_count << '\n';
        out << "comment count  : " << observer.comment_count << '\n';
        out << "error count    : " << observer.error_count << '\n';
        return out;
    }

    template <typename _Observer = counting_observer>
    class test
    {
    public:
        using observer_t = _Observer;
        using xml_data_t = std::vector<char>;

        test(const char* name, const char* data_path): file_paths_(xml_files(data_path)), name_(name) {}

        observer_t& observer() noexcept { return observer_; }

        void run()
        {
            duration_t duration {};
            for (const auto& file_path: file_paths_)
            {
                duration += parse(file_path);
            }

            std::cout << name_ << " parsing time: " << duration.count() << " s\n";
        }

    protected:
        using file_path_vector_t = std::vector<std::string>;
        using duration_t = std::chrono::duration<double>;

        virtual void execute(const xml_data_t& xml_data) = 0;

        duration_t parse(const std::string& file_path)
        {
            using namespace std;
            using namespace std::chrono;
            cout << "test begins: " << file_path << '\n';
            observer_.clear();
            xml_data_ = stdext::read_file(file_path);
            const auto start_time = high_resolution_clock::now();
            execute(xml_data_);
            const duration_t diff = high_resolution_clock::now() - start_time;
            xml_data_.clear();
            cout << observer_;
            cout << "test ends: " << file_path << '\n';
            return diff;
        }

    private:
        observer_t observer_ {};
        file_path_vector_t file_paths_;
        xml_data_t xml_data_;
        const char* name_;
    };
}

/// xpar - Event based XML parser
/// Copyright (c) Flaviu Cibu. All rights reserved.
/// Created 19-oct-2009
/// Updated 12-dec-2010
/// Updated 20-mar-2018

#pragma once
#ifndef PCH
    #include <array>
    #include <cstring>
#endif

namespace stdext
{
    /*class observer_example
    {
    public:
        void on_element_begin(xpar_t& parser, const char_t* name, const char_t* name_end);
        void on_element_end(xpar_t& parser, const char_t* name, const char_t* name_end);
        void on_attribute(xpar_t& parser, const char_t* name, const char_t* name_end);
        void on_attribute_value(xpar_t& parser, const char_t* text, const char_t* text_end, const bool partial);
        void on_data(xpar_t& parser, const char_t* text, const char_t* text_end, const bool partial);
        void on_comment(xpar_t& parser, const char_t* text, const char_t* text_end, const bool partial);
        void on_error(xpar_t& parser, bool& try_continue);
    };*/

    struct xpar_default_config
    {
        using char_t = char;

        enum constant
        {
            try_continue_on_error = 0,
            max_name_length = 32,
            max_value_length = 512,
            max_stack_size = 16
        };
    };

    template <typename _Observer, typename _Config = xpar_default_config>
    class xpar
    {
    public:
        using uint_t = unsigned;
        using config_t = _Config;
        using char_t = typename _Config::char_t;
        using observer_t = _Observer;

        enum class error_t
        {
            none,
            elem_end_not_match,
            unexpected_char,
            unterminated_comment,
            max_elem_name_length_exceeded,
            max_attr_name_length_exceeded,
            max_attr_value_length_exceeded,
        };

        constexpr xpar(observer_t* const observer) noexcept: observer_(observer) {}

        void operator()(const char_t* buffer, const std::size_t buffer_size);

        uint_t line() const noexcept { return line_; }
        uint_t column() const noexcept { return static_cast<uint_t>(1U + item_begin_ - line_begin_); }
        error_t error() const noexcept { return error_; }

        uint_t stack_size() const noexcept;
        const char_t* stack_value(const uint_t index) const noexcept;

        observer_t* observer() const noexcept { return observer_; }
        void set_observer(observer_t* const observer) noexcept { observer_ = observer; }

        void reset() noexcept;

    protected:
        enum class result_t
        {
            ok,
            more_data_required,
            limit_exceed,
        };

        enum class state_t
        {
            none,
            elem_handle,
            elem,
            elem_end,
            attr,
            attr_or_attr_value,
            expect_attr_value,
            attr_value,
            data,
            comment,
            meta,
            dtd,
            single_elem_end,
        };

        using value_buffer_t = std::array<char_t, config_t::max_value_length>;
        using stack_buffer_t = std::array<char_t, config_t::max_stack_size * config_t::max_name_length>;

        bool space() noexcept;
        result_t identifier() noexcept;
        void elem();
        void elem_handle();
        void attr();
        void attr_or_attr_value();
        void expect_attr_value();
        void attr_value();
        void attr_continue();
        void search_attr_value();
        void data();
        void parse();
        void elem_continue();
        void elem_end_continue();
        void search_elem_end();
        void single_elem_end();
        void attr_value_continue();
        void data_continue();
        void comment();
        void meta();
        void dtd();
        bool try_continue_handling_error(const error_t error);

        stack_buffer_t stack_buffer_ {};
        value_buffer_t value_buffer_ {};
        const char_t* ptr_ {};
        const char_t* end_ {};
        const char_t* line_begin_ {};
        const char_t* item_begin_ {};
        char_t* id_ {value_buffer_.data()};
        char_t* id_end_ {value_buffer_.data()};
        char_t* stack_pointer_ {stack_buffer_.data()};
        observer_t* observer_;
        uint_t line_ {1U};
        state_t state_ {};
        error_t error_ {};
        char_t last_delimiter_ {};
        bool item_read_ {};
    };

    template <typename _Observer, typename _Config>
    typename xpar<_Observer, _Config>::result_t xpar<_Observer, _Config>::identifier() noexcept
    {
        item_begin_ = ptr_;
        for (std::size_t len = id_end_ - id_;;)
        {
            if (ptr_ == end_) [[unlikely]]
                return result_t::more_data_required;
            switch (*ptr_)
            {
                case '_':
                case ':':
                case '-':
                    break;
                    [[likely]] default:
                    {
                        if (!::isalnum(*ptr_))
                            return result_t::ok;
                    }
            }
            *id_end_++ = *ptr_++;
            if (++len == config_t::max_name_length) [[unlikely]]
                return result_t::limit_exceed;
        }
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::operator()(const char_t* buffer, const std::size_t buffer_size)
    {
        ptr_ = buffer;
        end_ = buffer + buffer_size;
        if (!line_begin_)
            line_begin_ = ptr_;
        while ((ptr_ < end_) && (error_ == error_t::none))
            switch (state_)
            {
                case state_t::none:
                    parse();
                    break;
                case state_t::elem:
                    if (!item_read_)
                        elem_continue();
                    else
                        elem();
                    break;
                case state_t::elem_end:
                    elem_end_continue();
                    break;
                case state_t::elem_handle:
                    elem_handle();
                    break;
                case state_t::attr:
                    if (!item_read_)
                        attr_continue();
                    else
                        attr();
                    break;
                case state_t::attr_or_attr_value:
                    attr_or_attr_value();
                    break;
                case state_t::expect_attr_value:
                    expect_attr_value();
                    break;
                case state_t::attr_value:
                    attr_value_continue();
                    break;
                case state_t::data:
                    data_continue();
                    break;
                case state_t::single_elem_end:
                    single_elem_end();
                    break;
                default:
                    break;
            }
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::parse()
    {
        if (space())
        {
            if (*ptr_ == '<')
                elem();
            else
                data();
        }
    }

    template <typename _Observer, typename _Config>
    typename xpar<_Observer, _Config>::uint_t xpar<_Observer, _Config>::stack_size() const noexcept
    {
        return static_cast<uint_t>((stack_pointer_ - stack_buffer_.data()) / config_t::max_name_length);
    }

    template <typename _Observer, typename _Config>
    const typename xpar<_Observer, _Config>::char_t* xpar<_Observer, _Config>::stack_value(const uint_t index) const noexcept
    {
        const auto offset = config_t::max_name_length * index;
        return offset < stack_buffer_.size() ? stack_buffer_.data() + offset : nullptr;
    }

    template <typename _Observer, typename _Config>
    bool xpar<_Observer, _Config>::try_continue_handling_error(const error_t error)
    {
        error_ = error;
        bool try_continue = config_t::try_continue_on_error;
        observer_->on_error(*this, try_continue);
        return try_continue;
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::elem_continue()
    {
        switch (identifier())
        {
            [[likely]] case result_t::ok:
            {
                item_read_ = true;
                stack_pointer_ += config_t::max_name_length;
                std::memcpy(stack_pointer_, id_, id_end_ - id_);
                observer_->on_element_begin(*this, id_, id_end_);
                id_end_ = id_;
                state_ = state_t::attr;
                id_end_ = id_;
                attr();
                break;
            }
            case result_t::limit_exceed:
                if (try_continue_handling_error(error_t::max_elem_name_length_exceeded))
                {
                    error_ = {};
                    state_ = state_t::attr;
                    id_end_ = id_;
                    attr();
                }

                break;
            default:
                break;
        }
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::single_elem_end()
    {
        if (*ptr_ == '>') [[likely]]
        {
            ++ptr_;
            observer_->on_element_end(*this, nullptr, nullptr);
            // observer_->on_element_end( *this, id_, id_end_);
            stack_pointer_ -= config_t::max_name_length;
            state_ = {};
        }
        else if (try_continue_handling_error(error_t::unexpected_char))
        {
            error_ = {};
            ++ptr_;
        }
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::search_elem_end()
    {
        if (space() && (*ptr_ == '>')) [[likely]]
        {
            ++ptr_;
            item_read_ = true;
            if (std::memcmp((void*) stack_pointer_, id_, id_end_ - id_) == 0) [[likely]]
            {
                stack_pointer_ -= config_t::max_name_length;
                observer_->on_element_end(*this, id_, id_end_);
                state_ = {};
            }
            else if (try_continue_handling_error(error_t::elem_end_not_match))
            {
                stack_pointer_ -= config_t::max_name_length;
                state_ = {};
                error_ = {};
            }
        }
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::elem_end_continue()
    {
        switch (identifier())
        {
            [[likely]] case result_t::ok:
            {
                search_elem_end();
                break;
            }
            case result_t::limit_exceed:
                if (try_continue_handling_error(error_t::max_elem_name_length_exceeded))
                {
                    error_ = {};
                    search_elem_end();
                }
                break;
            default:
                break;
        }
    }

    template <typename _Observer, typename _Config>
    bool xpar<_Observer, _Config>::space() noexcept
    {
        while (ptr_ < end_)
            switch (*ptr_)
            {
                case '\r':
                    ++ptr_;
                    if (ptr_ != end_) [[likely]]
                    {
                        if (*ptr_ == '\n')
                            ++ptr_;

                        line_begin_ = ptr_;
                        ++line_;
                    }
                    break;
                case '\n':
                    ++ptr_;
                    ++line_;
                    line_begin_ = ptr_;
                    break;
                    [[likely]] case ' ': case '\t': case '\v':
                    {
                        ++ptr_;
                        break;
                    }
                default:
                    return true;
            }

        return false;
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::elem_handle()
    {
        switch (*ptr_)
        {
            case '/':
                state_ = state_t::elem_end;
                while (++ptr_ < end_)
                    if (::isalpha(*ptr_)) [[likely]]
                    {
                        elem_end_continue();
                        break;
                    }
                    else if (try_continue_handling_error(error_t::unexpected_char))
                        error_ = {};
                    else
                        break;
                break;
            case '?':
                state_ = state_t::meta;
                ++ptr_;
                meta();
                break;
            case '!':
                ++ptr_;
                if (ptr_ < end_) [[likely]]
                {
                    if (*ptr_ == '-') [[likely]]
                    {
                        state_ = state_t::comment;
                        comment();
                    }
                    else
                    {
                        state_ = state_t::dtd;
                        dtd();
                    }
                }
                break;
                [[likely]] default:
                {
                    for (;;)
                        if (::isalpha(*ptr_)) [[likely]]
                        {
                            state_ = state_t::elem;
                            elem_continue();
                            break;
                        }
                        else if (try_continue_handling_error(error_t::unexpected_char))
                        {
                            error_ = {};
                            state_ = state_t::elem;
                            if (++ptr_ >= end_)
                                break;
                        }
                        else
                            break;
                    break;
                }
        }
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::comment()
    {
        const char_t* text = ptr_;
        for (uint_t count = 1U; ptr_ < end_; ++ptr_)
            switch (*ptr_)
            {
                case '-':
                    ++count;
                    break;
                case '>':
                    if (count == 2U)
                    {
                        ++ptr_;
                        state_ = {};
                        observer_->on_comment(*this, text, ptr_, ptr_ == end_);
                        return;
                    }
                    else if (try_continue_handling_error(error_t::unterminated_comment))
                    {
                        count = 0U;
                        ++ptr_;
                        error_ = {};
                    }
                    break;
                    [[likely]] default:
                    {
                        if (count != 0U)
                        {
                            count = 0U;
                        }

                        break;
                    }
            }
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::meta()
    {
        for (; ptr_ < end_; ++ptr_)
            if (*ptr_ == '>')
            {
                if (ptr_[-1] == '?')
                {
                    ++ptr_;
                    state_ = {};
                    id_end_ = id_;
                    break;
                }
                else if (try_continue_handling_error(error_t::unexpected_char))
                {
                    error_ = {};
                    ++ptr_;
                }
                else
                    break;
            }
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::dtd()
    {
        uint_t inner_elems = 1u;
        for (; ptr_ < end_; ++ptr_)
            switch (*ptr_)
            {
                case '>':
                    --inner_elems;
                    if (inner_elems == 0u)
                    {
                        state_ = {};
                        return;
                    }
                    break;
                case '<':
                    ++inner_elems;
                    break;
                    [[likely]] default:
                    {
                        break;
                    }
            }

        if (inner_elems != 0U) [[unlikely]]
        {
            if (try_continue_handling_error(error_t::elem_end_not_match))
            {
                error_ = {};
                ++ptr_;
            }
        }
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::elem()
    {
        item_read_ = false;
        id_end_ = id_;
        ++ptr_;
        if (ptr_ < end_) [[likely]]
            elem_handle();
        else
            state_ = state_t::elem_handle;
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::expect_attr_value()
    {
        while (space())
        {
            if ((*ptr_ == '"') || (*ptr_ == '\'')) [[likely]]
            {
                attr_value();
                break;
            }
            else if (try_continue_handling_error(error_t::unexpected_char))
            {
                ++ptr_;
                error_ = {};
            }
            else
                break;
        }
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::search_attr_value()
    {
        if (space() && (*ptr_ == '=')) [[likely]]
        {
            ++ptr_;
            state_ = state_t::expect_attr_value;
            expect_attr_value();
        }
        else
            attr();
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::attr_continue()
    {
        switch (identifier())
        {
            [[likely]] case result_t::ok:
            {
                item_read_ = true;
                observer_->on_attribute(*this, id_, id_end_);
                search_attr_value();
                break;
            }
            case result_t::limit_exceed:
                if (try_continue_handling_error(error_t::max_attr_name_length_exceeded))
                {
                    error_ = {};
                    search_attr_value();
                }
                break;
            default:
                break;
        }
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::attr()
    {
        while ((ptr_ < end_) && (error_ == error_t::none))
        {
            switch (*ptr_)
            {
                case ' ':
                case '\t':
                case '\v':
                case '\r':
                case '\n':
                    space();
                    break;
                case '>':
                    ++ptr_;
                    state_ = {};
                    return;
                case '/':
                    ++ptr_;
                    state_ = state_t::single_elem_end;
                    if (ptr_ < end_)
                        single_elem_end();
                    return;
                    [[likely]] default:
                    {
                        if (::isalpha(*ptr_)) [[likely]]
                        {
                            id_end_ = id_;
                            item_read_ = false;
                            attr_continue();
                        }
                        else if (try_continue_handling_error(error_t::unexpected_char))
                        {
                            error_ = {};
                            ++ptr_;
                        }

                        break;
                    }
            }
        }
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::attr_or_attr_value()
    {
        if (space())
        {
            if (::isalpha(*ptr_)) [[likely]]
                attr();
            else if (*ptr_ == '=')
            {
                attr_value();
                attr();
            }
            else [[unlikely]] if (try_continue_handling_error(error_t::unexpected_char))
            {
                error_ = {};
                ++ptr_;
            }
        }
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::attr_value()
    {
        last_delimiter_ = *ptr_;
        ++ptr_;
        id_end_ = id_;
        state_ = state_t::attr_value;
        attr_value_continue();
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::attr_value_continue()
    {
        std::size_t len = id_end_ - id_;
        const std::size_t limit = std::min<std::size_t>(end_ - ptr_, config_t::max_value_length);
        for (; (len < limit) && (*ptr_ != last_delimiter_); ++len)
            *id_end_++ = *ptr_++;

        if (len == config_t::max_value_length) [[unlikely]]
        {
            if (try_continue_handling_error(error_t::max_attr_value_length_exceeded))
            {
                id_end_ = id_;
                error_ = {};
                while ((ptr_ < end_) && (*ptr_ != '"') && (*ptr_ != '\''))
                    ++ptr_;
            }
            else
                return;
        }

        if ((ptr_ < end_) && (*ptr_ == last_delimiter_)) [[likely]]
        {
            observer_->on_attribute_value(*this, id_, id_end_, false);
            last_delimiter_ = {};
            ++ptr_;
            state_ = state_t::attr;
        }
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::data()
    {
        state_ = state_t::data;
        data_continue();
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::data_continue()
    {
        const char_t* const text = ptr_;
        while (ptr_ < end_)
            switch (*ptr_)
            {
                case '\r':
                case '\n':
                    space();
                    break;
                case '<':
                    goto exit;
                    [[likely]] default:
                    {
                        ++ptr_;
                        break;
                    }
            }
exit:
        const bool end = ptr_ == end_;
        if (ptr_ > text)
            observer_->on_data(*this, text, ptr_, end);

        if (!end)
            elem();
    }

    template <typename _Observer, typename _Config>
    void xpar<_Observer, _Config>::reset() noexcept
    {
        line_begin_ = {};
        item_begin_ = {};
        line_ = 1U;
        state_ = {};
        error_ = {};
        item_read_ = {};
    }
}

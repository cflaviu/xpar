/// xpar - Event based XML parser
/// Copyright (c) Flaviu Cibu. All rights reserved.
/// Created 19-oct-2009
/// Updated 12-dec-2010
/// Updated 20-mar-2018

#pragma once
#ifndef PCH
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
		typedef char char_t;
		static const char delimiter = '\"';

		enum const_t
		{
			max_name_length = 32,
			max_value_length = 512,
			max_stack_size  = 16
		};
	};

	template <typename _Observer, typename _Config = xpar_default_config>
	class xpar
	{
	public:
		typedef unsigned uint;
		typedef unsigned char byte;
		typedef _Config config_t;
		typedef typename _Config::char_t char_t;
		typedef _Observer observer_t;

		enum error_t
		{
			err_none,
			err_elem_end_not_match,
			err_unexpected_char,
			err_max_elem_name_length_exceeded,
			err_max_attr_name_length_exceeded,
			err_max_attr_value_length_exceeded
		};

		xpar(observer_t* const observer);

		void operator () (const char_t* buffer, const size_t bufferSize);
		void operator () (const char_t* buffer) { operator () (buffer, strlen(buffer)); }

		uint line() const { return line_; }
		uint column() const { return uint(1 + item_ptr_ - line_ptr_); }
		error_t error() const { return error_; }
		uint deep() const { return uint((stack_ - (char_t*)buffer_ + config_t::max_name_length) / config_t::max_name_length); }

		observer_t* observer() { return observer_; }
		void set_observer(observer_t* const observer) { if (observer) observer_ = observer; }

		void reset();

	protected:
		enum result_t
		{
			res_ok,
			res_more_data_required,
			res_exceed_limit
		};

		enum state_t
		{
			st_none,
			st_elem_handle,
			st_elem,
			st_elem_end,
			st_attr,
			st_attr_or_attr_value,
			st_expect_attr_value,
			st_attr_value,
			st_data,
			st_comment,
			st_meta,
			st_single_elem_end
		};

		bool space();
		result_t identifier(const size_t limit);
		void elem();
		void elem_handle();
		void attr();
		void attr_or_attr_value();
		void expect_attr_value();
		void begin_attr_value();
		void attr_value();
		void attr_continue();
		void data();
		void parse();
		void elem_continue();
		void elem_end_continue();
		void single_elem_end();
		void attr_value_continue();
		void data_continue();
		void comment();
		void meta();

		const char_t*	ptr_;
		const char_t*	end_;
		const char_t*	line_ptr_;
		const char_t*	item_ptr_;
		char_t*			id_;
		char_t*			id_end_;
		char_t*			stack_;
		state_t			state_;
		error_t			error_;
		uint			line_;
		observer_t*		observer_;
		bool			item_read_;
		char_t 			buffer_[config_t::max_value_length + config_t::max_stack_size * config_t::max_name_length];
	};

	template <typename _Observer, typename _Config>
	xpar<_Observer, _Config>::xpar(observer_t* const observer) :
		ptr_(nullptr),
		end_(nullptr),
		line_ptr_(nullptr),
		item_ptr_(nullptr),
		id_(static_cast<char_t*>(buffer_)),
		id_end_(id_),
		stack_(id_ + config_t::max_value_length),
		state_(st_none),
		error_(err_none),
		line_(1),
		observer_(observer),
		item_read_(false)
	{}

	template <typename _Observer, typename _Config>
	typename xpar<_Observer, _Config>::result_t xpar<_Observer, _Config>::identifier(const size_t limit)
	{
		item_ptr_ = ptr_;
		result_t res = res_ok;
		size_t len = id_end_ - id_;
		while (1)
		{
			if (ptr_ == end_)
			{
				res = res_more_data_required;
				break;
			}

			switch (*ptr_)
			{
				case '_':
				case ':':
				case '-': break;
				default:
				{
					if (!isalnum(*ptr_))
					{
						goto exit;
					}
				}
			}

			*id_end_++ = *ptr_++;
			if (++len == limit)
			{
				res = res_exceed_limit;
				break;
			}
		}
	exit:
		return res;
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::operator () (const typename xpar<_Observer, _Config>::char_t* buffer, const size_t bufferSize)
	{
		if (!observer_) return;

		ptr_ = buffer;
		end_ = buffer + bufferSize;
		if (!line_ptr_) line_ptr_ = ptr_;

		while (ptr_ < end_ && !error_)
		{
			switch (state_)
			{
				case st_none: parse(); break;
				case st_elem:
				{
					if (!item_read_)
					{
						elem_continue();
					}
					else
					{
						elem();
					}
					break;
				}
				case st_elem_end: elem_end_continue(); break;
				case st_elem_handle: elem_handle(); break;
				case st_attr:
				{
					if (!item_read_)
					{
						attr_continue();
					}
					else
					{
						attr();
					}
					break;
				}
				case st_attr_or_attr_value: attr_or_attr_value(); break;
				case st_expect_attr_value: expect_attr_value(); break;
				case st_attr_value: attr_value_continue(); break;
				case st_data: data_continue(); break;
				case st_single_elem_end: single_elem_end(); break;
			}
		}
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::parse()
	{
		if (space())
		{
			switch (*ptr_)
			{
				case '<': elem(); break;
				default: data();
			}
		}
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::elem_continue()
	{
		switch (identifier(config_t::max_name_length))
		{
			case res_ok:
			{
				item_read_ = true;

				stack_ += config_t::max_name_length;
				memcpy(stack_, id_, id_end_ - id_);

				observer_->on_element_begin(*this, id_, id_end_);
				id_end_ = id_;

			restart:
				state_ = st_attr;
				id_end_ = id_;
				attr();
				break;
			}

			case res_more_data_required: break;
			case res_exceed_limit:
			{
				error_ = err_max_elem_name_length_exceeded;
				bool try_continue = true;
				observer_->on_error(*this, try_continue);
				if (try_continue)
				{
					error_ = err_none;
					goto restart;
				}

				break;
			}
		}
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::single_elem_end()
	{
		if (*ptr_ == '>')
		{
			++ptr_;
			observer_->on_element_end(*this, nullptr, nullptr);
			//observer_->on_element_end( *this, id_, id_end_);
			stack_ -= config_t::max_name_length;
			state_ = st_none;
		}
		else
		{
			error_ = err_unexpected_char;
			bool try_continue = true;
			observer_->on_error(*this, try_continue);
			if (try_continue)
			{
				error_ = err_none;
				++ptr_;
			}
		}
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::elem_end_continue()
	{
		switch (identifier(config_t::max_name_length))
		{
			case res_ok:
			{
			restart:
				if (space())
				{
					if (*ptr_ == '>')
					{
						++ptr_;
						item_read_ = true;
						if (!memcmp((void*)stack_, id_, id_end_ - id_))
						{
							stack_ -= config_t::max_name_length;
							observer_->on_element_end(*this, id_, id_end_);
							state_ = st_none;
						}
						else
						{
							error_ = err_elem_end_not_match;
							bool try_continue = true;
							observer_->on_error(*this, try_continue);
							if (try_continue)
							{
								error_ = err_none;
								stack_ -= config_t::max_name_length;
								state_ = st_none;
							}
						}
					}
				}
				break;
			}

			case res_more_data_required: break;
			case res_exceed_limit:
			{
				error_ = err_max_elem_name_length_exceeded;
				bool try_continue = true;
				observer_->on_error(*this, try_continue);
				if (try_continue)
				{
					error_ = err_none;
					goto restart;
				}
				break;
			}
		}
	}

	template <typename _Observer, typename _Config>
	bool xpar<_Observer, _Config>::space()
	{
		while (ptr_ < end_)
		{
			switch (*ptr_)
			{
				case '\r':
				{
					++ptr_;
					if (ptr_ != end_)
					{
						if (*ptr_ == '\n')
						{
							++ptr_;
						}
						line_ptr_ = ptr_;
						++line_;
					}
					break;
				}

				case '\n':
				{
					++ptr_;
					++line_;
					line_ptr_ = ptr_;
					break;
				}

				case ' ':
				case '\t':
				case '\v': ++ptr_; break;
				default: return true;
			}
		}

		return false;
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::elem_handle()
	{
		switch (*ptr_)
		{
			case '/':
			{
				state_ = st_elem_end;
			restart1:
				if (++ptr_ < end_)
				{
					if (isalpha(*ptr_))
					{
						elem_end_continue();
					}
					else
					{
						error_ = err_unexpected_char;
						bool try_continue = true;
						observer_->on_error(*this, try_continue);
						if (try_continue)
						{
							++ptr_;
							error_ = err_none;
							goto restart1;
						}
					}
				}
				break;
			}
			case '?':
			{
				state_ = st_meta;
				++ptr_;
				meta();
				break;
			}
			case '!':
			{
				state_ = st_comment;
				++ptr_;
				comment();
				break;
			}
			default:
			{
			restart2:
				if (isalpha(*ptr_))
				{
					state_ = st_elem;
					elem_continue();
				}
				else
				{
					error_ = err_unexpected_char;
					bool try_continue = true;
					observer_->on_error(*this, try_continue);
					if (try_continue)
					{
						error_ = err_none;
						state_ = st_elem;
						if (++ptr_ < end_)
						{
							goto restart2;
						}
					}
				}
			}
		}
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::comment()
	{
		const char_t* text = ptr_;
		int count = 0;
		while (ptr_ < end_)
		{
			switch (*ptr_)
			{
				case '-':
				{
					++count;
					break;
				}
				case '>':
				{
					if (count == 2)
					{
						++ptr_;
						state_ = st_none;
						goto exit;
					}
					break;
				}
				default:
				{
					count = 0;
				}
			}
			++ptr_;
		}
	exit:;
		observer_->on_comment(*this, text, ptr_, ptr_ == end_);
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::meta()
	{
	restart:
		while (ptr_ < end_)
		{
			switch (*ptr_)
			{
				case '>':
				{
					if (ptr_[-1] == '?')
					{
						++ptr_;
						state_ = st_none;
						id_end_ = id_;
					}
					else
					{
						error_ = err_unexpected_char;
						bool try_continue = true;
						observer_->on_error(*this, try_continue);
						if (try_continue)
						{
							error_ = err_none;
							++ptr_;
							goto restart;
						}
					}
					goto exit;
				}
			}
			++ptr_;
		}
	exit:;
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::elem()
	{
		item_read_ = false;
		id_end_ = id_;
		++ptr_;
		if (ptr_ < end_)
		{
			elem_handle();
		}
		else
		{
			state_ = st_elem_handle;
		}
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::expect_attr_value()
	{
	restart:
		if (space())
		{
			if (*ptr_ == config_t::delimiter)
			{
				attr_value();
			}
			else
			{
				error_ = err_unexpected_char;
				bool try_continue = true;
				observer_->on_error(*this, try_continue);
				if (try_continue)
				{
					error_ = err_none;
					++ptr_;
					goto restart;
				}
			}
		}
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::attr_continue()
	{
		switch (identifier(config_t::max_name_length))
		{
			case res_ok:
			{
				item_read_ = true;
				observer_->on_attribute(*this, id_, id_end_);
			restart:
				if (space() && *ptr_ == '=')
				{
					++ptr_;
					state_ = st_expect_attr_value;
					expect_attr_value();
				}
				else
				{
					attr();
				}

				break;
			}

			case res_more_data_required: break;
			case res_exceed_limit:
			{
				error_ = err_max_attr_name_length_exceeded;
				bool try_continue = true;
				observer_->on_error(*this, try_continue);
				if (try_continue)
				{
					error_ = err_none;
					goto restart;
				}
			}
		}
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::attr()
	{
		while (ptr_ < end_ && !error_)
		{
			switch (*ptr_)
			{
				case ' ':
				case '\t':
				case '\v':
				case '\r':
				case '\n':
				{
					space();
					break;
				}
				case '>':
				{
					++ptr_;
					state_ = st_none;
					goto exit;
				}
				case '/':
				{
					state_ = st_single_elem_end;
					++ptr_;
					if (ptr_ < end_)
					{
						single_elem_end();
					}
					goto exit;
				}
				default:
				{
					if (isalpha(*ptr_))
					{
						id_end_ = id_;
						item_read_ = false;
						attr_continue();
					}
					else
					{
						error_ = err_unexpected_char;
						bool try_continue = true;
						observer_->on_error(*this, try_continue);
						if (try_continue)
						{
							error_ = err_none;
							++ptr_;
						}
					}
				}
			}
		}
	exit:;
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::attr_or_attr_value()
	{
		if (space())
		{
			if (isalpha(*ptr_))
			{
				attr();
			}
			else if (*ptr_ == '=')
			{
				attr_value();
				attr();
			}
			else
			{
				error_ = err_unexpected_char;
				bool try_continue = true;
				observer_->on_error(*this, try_continue);
				if (try_continue)
				{
					error_ = err_none;
					++ptr_;
				}
			}
		}
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::attr_value()
	{
		++ptr_;
		id_end_ = id_;
		state_ = st_attr_value;
		attr_value_continue();
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::attr_value_continue()
	{
		size_t len = id_end_ - id_;
		while (ptr_ < end_ && *ptr_ != config_t::delimiter && len < config_t::max_value_length)
		{
			*id_end_++ = *ptr_++;
			++len;
		}

		if (len == config_t::max_value_length)
		{
			error_ = err_max_attr_value_length_exceeded;

			bool try_continue = true;
			observer_->on_error(*this, try_continue);
			if (try_continue)
			{
				id_end_ = id_;
				error_ = err_none;
				while (ptr_ < end_ && *ptr_ != config_t::delimiter)
				{
					++ptr_;
				}
			}
			else
			{
				return;
			}
		}

		if (ptr_ < end_ && *ptr_ == config_t::delimiter)
		{
			observer_->on_attribute_value(*this, id_, id_end_, false);
			++ptr_;
			state_ = st_attr;
		}
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::data()
	{
		state_ = st_data;
		data_continue();
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::data_continue()
	{
		const char_t* text = ptr_;
		while (ptr_ < end_)
		{
			switch (*ptr_)
			{
				case '\r':
				case '\n':
				{
					space();
					break;
				}
				case '<':
				{
					goto exit;
				}
				default: ++ptr_;
			}
		}

	exit:
		const bool end = ptr_ == end_;
		if (ptr_ > text)
		{
			observer_->on_data(*this, text, ptr_, end);
		}

		if (!end)
		{
			elem();
		}
	}

	template <typename _Observer, typename _Config>
	void xpar<_Observer, _Config>::reset()
	{
		line_ptr_ = nullptr;
		item_ptr_ = nullptr;
		state_ = st_none;
		item_read_ = false;
		line_ = 1;
		error_ = err_none;
	}
}

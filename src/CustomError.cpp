// Copyright (c) embedded ocean GmbH
#include "CustomError.hpp"

#include <system_error>
#include <string>

namespace xentara::plugins::templateUplink
{

namespace
{

	// This is a custom error category to encapsulate ErrorCode errors in std::error_code objects
	class ErrorCategory final : public std::error_category
	{
	public:
		// Override that returns a name for the category
		auto name() const noexcept -> const char * final;

		// Override that returns an error message
		auto message(int errorCode) const -> std::string final;
	};

	auto ErrorCategory::name() const noexcept -> const char *
	{
		// TODO: Rename error cotegory
		return "Xentara.templateUplink";
	}

	auto ErrorCategory::message(int errorCode) const -> std::string
	{
		using namespace std::literals;

		// Check the error code
		switch (CustomError(errorCode))
		{
		case CustomError::NoError:
			return "success"s;

		case CustomError::NotConnected:
			// TODO: Make error more descriptive
			return "the client is not connected"s;

		// TODO: Add messages for other error codes here

		case CustomError::UnknownError:
		default:
			return "an unknown error occurred"s;
		}
	}

} // namespace

// Support automatic conversion from CustomError to std::error_code
auto customErrorCategory() noexcept -> const std::error_category &
{
	// Return a static object
	static const ErrorCategory kErrorCategory;
	return kErrorCategory;
}

} // namespace xentara::plugins::templateUplink
// Copyright (c) embedded ocean GmbH
#pragma once

#include <system_error>
#include <type_traits>

// TODO: rename namespace
namespace xentara::plugins::templateUplink
{

// Custom error codes used by the driver
enum class CustomError
{
	// The base value for custom errors, so they don't overlap with system errors
	NoError = 0,

	// The device connection is not open.
	// TODO: change this to NotOpen for devices that are not remotely connected
	NotConnected,

	// TODO: Add more error codes here

	// An unknown error occurred
	UnknownError = 999
};

// The error category for custom errors
auto customErrorCategory() noexcept -> const std::error_category &;

// Support automatic conversion from CustomError to std::error_code
inline auto make_error_code(CustomError error) noexcept -> std::error_code
{
	return { int(error), customErrorCategory() };
}

} // namespace xentara::plugins::templateUplink

namespace std
{

// This specialization enables automatic conversion from CustomError to std::error_code.
template<>
struct is_error_code_enum<xentara::plugins::templateUplink::CustomError> : public std::true_type
{
};

}
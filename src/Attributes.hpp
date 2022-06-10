// Copyright (c) embedded ocean GmbH
#pragma once

#include "CustomError.hpp"

#include <xentara/model/Attribute.hpp>

#include <cstdint>
#include <system_error>

// TODO: rename namespace
namespace xentara::plugins::templateUplink::attributes
{

// A Xentara attribute containing the state of a client connection
extern const model::Attribute kConnectionState;
// A Xentara attribute containing the state of a transaction
extern const model::Attribute kTransactionState;

// A Xentara attribute containing the connection time for a client
extern const model::Attribute kConnectionTime;
// A Xentara attribute containing the send time for a transaction
extern const model::Attribute kSendTime;

// A Xentara attribute containing an error code for a client connection
extern const model::Attribute kError;

// TODO: add any additional attributes supplied by the individual elements

// The type used for error codes in kError and kWriteError
using ErrorCode = std::uint32_t;

// Generates an error code for a custom error
auto errorCode(CustomError error) noexcept -> ErrorCode;

// TODO: add more functions for other categories of errors, e.g. error codes from a library or protocol

// Generates an error code for an std::error_code
auto errorCode(std::error_code error) noexcept -> ErrorCode;

} // namespace xentara::plugins::templateUplink::attributes
// Copyright (c) embedded ocean GmbH
#pragma once

#include <xentara/config/Resolver.hpp>
#include <xentara/data/ReadHandle.hpp>
#include <xentara/model/GenericElement.hpp>
#include <xentara/utils/core/RawDataBlock.hpp>
#include <xentara/utils/json/decoder/Value.hpp>

#include <chrono>
#include <memory>
#include <string>

// TODO: rename namespace
namespace xentara::plugins::templateUplink
{

// A class representing a single record in a transaction.
// TODO: rename this class to something more descriptive
class TemplateRecord final
{
public:
	// Loads the record from a JSON value
	auto loadConfig(utils::json::decoder::Value &value, config::Resolver &resolver) -> void;

	// Collects the data from the record and appends it to a data block
	auto collect(std::chrono::system_clock::time_point timeStamp, utils::core::RawDataBlock &data) const -> void;

	// Reslves read handles
	auto resolveHandles() -> void;

private:
	// The data point
	std::weak_ptr<const model::GenericElement> _dataPoint;
	// The ID of the record in the namespace of the remote service
	// TODO: use the appropriate type here
	// TODO: rename the variable into something specific to the key used by the remote service, like e.g. "objectName"
	std::u8string _remoteId;

	// TODO: add more properties needed for the record, e.g. data type

	// The read handle for the value
	data::ReadHandle _valueReadHandle;
	// The read handle for the quality
	data::ReadHandle _qualityReadHandle;

	// TODO: add read handles for other attributes that should be sent
};

} // namespace xentara::plugins::templateUplink

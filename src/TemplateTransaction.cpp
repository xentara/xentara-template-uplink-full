// Copyright (c) embedded ocean GmbH
#include "TemplateTransaction.hpp"

#include "Attributes.hpp"

#include <xentara/data/DataType.hpp>
#include <xentara/data/ReadHandle.hpp>
#include <xentara/data/WriteHandle.hpp>
#include <xentara/memory/WriteSentinel.hpp>
#include <xentara/model/Attribute.hpp>
#include <xentara/process/ExecutionContext.hpp>
#include <xentara/utils/json/decoder/Object.hpp>
#include <xentara/utils/json/decoder/Errors.hpp>
#include <xentara/utils/eh/currentErrorCode.hpp>

#include <concepts>

namespace xentara::plugins::templateUplink
{
	
using namespace std::literals;

TemplateTransaction::Class TemplateTransaction::Class::_instance;

auto TemplateTransaction::loadConfig(const ConfigIntializer &initializer,
		utils::json::decoder::Object &jsonObject,
		config::Resolver &resolver,
		const FallbackConfigHandler &fallbackHandler) -> void
{
	// Get a reference that allows us to modify our own config attributes
    auto &&configAttributes = initializer[Class::instance().configHandle()];

	// Go through all the members of the JSON object that represents this object
	for (auto && [name, value] : jsonObject)
    {
		// TODO: rename member to something suitable for the specific client
		if (name == u8"records")
		{
			// Go through all the elements
			for (auto &&element : value.asArray())
			{
				// Add a record
				_records.emplace_front();
				// Load it
				_records.front().loadConfig(element, resolver);
			}
		}
		// TODO: load configuration parameters
		else if (name == u8"TODO"sv)
		{
			// TODO: parse the value correctly
			auto todo = value.asNumber<std::uint64_t>();

			// TODO: check that the value is valid
			if (!"TODO")
			{
				// TODO: use an error message that tells the user exactly what is wrong
				utils::json::decoder::throwWithLocation(value, std::runtime_error("TODO is wrong with TODO parameter of template transaction"));
			}

			// TODO: set the appropriate member variables, and update configAttributes accordingly (if necessary) 
		}
		else
		{
			// Pass any unknown parameters on to the fallback handler, which will load the built-in parameters ("id", "uuid", and "children"),
			// and throw an exception if the key is unknown
            fallbackHandler(name, value);
		}
    }

	// TODO: perform consistency and completeness checks
	if (!"TODO")
	{
		// TODO: use an error message that tells the user exactly what is wrong
		utils::json::decoder::throwWithLocation(jsonObject, std::runtime_error("TODO is wrong with template transaction"));
	}
}

auto TemplateTransaction::performCollectTask(const process::ExecutionContext &context) -> void
{
	// Collect the data
	collectData(context.scheduledTime());
}

auto TemplateTransaction::collectData(std::chrono::system_clock::time_point timeStamp) -> void
{
	// Go through all the records and collect the data
	for (auto &&record : _records)
	{
		record.collect(timeStamp,  _pendingData);
	}
}

auto TemplateTransaction::performSendTask(const process::ExecutionContext &context) -> void
{
	// Only perform the read only if the client is connected
	if (!_client.get().connected())
	{
		// Clear any pending data, so it doesn't accumulate indefinitely
		_pendingData.clear();

		return;
	}

	// Send the data
	send(context.scheduledTime());
}

auto TemplateTransaction::send(std::chrono::system_clock::time_point timeStamp) -> void
{
	// See if we even have data
	if (_pendingData.empty())
	{
		return;
	}

	// Take the data
	decltype(_pendingData) data;
	// Note: std::ranges::swap honours customization points and is thus preferrable to than std::swap
	std::ranges::swap(_pendingData, data);
	
	try
	{
		// TODO: send the data

		// TODO: if the data function does not throw errors, but uses return types or internal handle state,
		// throw an std::system_error here on failure, or call handleSendError() directly.

		// The write was successful
		updateState(timeStamp);

		// Fire the event
		_sentEvent.fire();
	}
	catch (const std::exception &)
	{
		// Get the error from the current exception using this special utility function
		const auto error = utils::eh::currentErrorCode();
		// Update the state
		handleSendError(timeStamp, error);
	}
}

auto TemplateTransaction::handleSendError(std::chrono::system_clock::time_point timeStamp, std::error_code error)
	-> void
{
	// Update our own state
	updateState(timeStamp, error);
	// Notify the client
	_client.get().handleError(timeStamp, error, this);
		
	// Send the event
	_sendErrorEvent.fire();
}

auto TemplateTransaction::updateState(std::chrono::system_clock::time_point timeStamp, std::error_code error) -> void
{
	// Make a write sentinel
	memory::WriteSentinel sentinel { _stateDataBlock };
	auto &state = *sentinel;

	// Update the state
	state._transactionState = !error;
	state._sendTime = timeStamp;
	state._error = attributes::errorCode(error);

	// Commit the data
	sentinel.commit();
}

auto TemplateTransaction::resolveAttribute(std::u16string_view name) -> const model::Attribute *
{
	// TODO: add any additional attributes this class supports, including attributes inherited from the client
	return model::Attribute::resolve(name,
		attributes::kTransactionState,
		attributes::kSendTime,
		attributes::kError);
}

auto TemplateTransaction::resolveTask(std::u16string_view name) -> std::shared_ptr<process::Task>
{
	if (name == u"collect"sv)
	{
		return std::shared_ptr<process::Task>(sharedFromThis(), &_collectTask);
	}
	else if (name == u"send"sv)
	{
		return std::shared_ptr<process::Task>(sharedFromThis(), &_sendTask);
	}

	// TODO resolve any additional tasks

	return nullptr;
}

auto TemplateTransaction::resolveEvent(std::u16string_view name) -> std::shared_ptr<process::Event>
{
	// Check all the events we support
	if (name == u"sent"sv)
	{
		return std::shared_ptr<process::Event>(sharedFromThis(), &_sentEvent);
	}
	else if (name == u"sendError"sv)
	{
		return std::shared_ptr<process::Event>(sharedFromThis(), &_sendErrorEvent);
	}

	// TODO resolve any additional events

	return nullptr;
}

auto TemplateTransaction::readHandle(const model::Attribute &attribute) const noexcept -> data::ReadHandle
{
	// Try our attributes
	if (attribute == attributes::kTransactionState)
	{
		return _stateDataBlock.member(&State::_transactionState);
	}
	else if (attribute == attributes::kSendTime)
	{
		return _stateDataBlock.member(&State::_sendTime);
	}
	else if (attribute == attributes::kError)
	{
		return _stateDataBlock.member(&State::_error);
	}

	// TODO add support for any additional attributes, including attributes inherited from the client

	return data::ReadHandle::Error::Unknown;
}

auto TemplateTransaction::realize() -> void
{
	// Create the data block
	_stateDataBlock.create(memory::memoryResources::data());
}

auto TemplateTransaction::prepare() -> void
{
	// Resolve all the handles for the records
	for (auto &&record : _records)
	{
		record.resolveHandles();
	}
}

auto TemplateTransaction::clientStateChanged(std::chrono::system_clock::time_point timeStamp, std::error_code error) -> void
{
	// Simply update the state
	updateState(timeStamp, error);
}

auto TemplateTransaction::CollectTask::preparePreOperational(const process::ExecutionContext &context) -> Status
{
	// We don't actually need to do anything, so just tell the scheduler that it can proceed to the next
	// stage as far as we're concered
	return Status::Ready;
}

auto TemplateTransaction::CollectTask::preOperational(const process::ExecutionContext &context) -> Status
{
	// We just do the same thing as in the operational stage
	operational(context);

	return Status::Ready;
}

auto TemplateTransaction::CollectTask::operational(const process::ExecutionContext &context) -> void
{
	// read the value
	_target.get().performCollectTask(context);
}

auto TemplateTransaction::SendTask::preparePreOperational(const process::ExecutionContext &context) -> Status
{
	// Request a connection
	_target.get().requestConnect(context.scheduledTime());

	return Status::Ready;
}

auto TemplateTransaction::SendTask::preOperational(const process::ExecutionContext &context) -> Status
{
	// We just do the same thing as in the operational stage
	operational(context);

	return Status::Ready;
}

auto TemplateTransaction::SendTask::operational(const process::ExecutionContext &context) -> void
{
	// read the value
	_target.get().performSendTask(context);
}

auto TemplateTransaction::SendTask::preparePostOperational(const process::ExecutionContext &context) -> Status
{
	// Send any remaining data
	_target.get().performSendTask(context);

	// Request a disconnect
	_target.get().requestDisconnect(context.scheduledTime());

	return Status::Completed;
}

} // namespace xentara::plugins::templateUplink
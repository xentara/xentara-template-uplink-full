// Copyright (c) embedded ocean GmbH
#include "TemplateClient.hpp"

#include "Attributes.hpp"
#include "TemplateTransaction.hpp"

#include <xentara/data/ReadHandle.hpp>
#include <xentara/memory/WriteSentinel.hpp>
#include <xentara/model/Attribute.hpp>
#include <xentara/plugin/SharedFactory.hpp>
#include <xentara/process/ExecutionContext.hpp>
#include <xentara/utils/eh/currentErrorCode.hpp>
#include <xentara/utils/json/decoder/Errors.hpp>
#include <xentara/utils/json/decoder/Object.hpp>

#include <string_view>

#ifdef _WIN32
#	include <Windows.h>
#else
#	include <errno.h>
#endif

namespace xentara::plugins::templateUplink
{

using namespace std::literals;

TemplateClient::Class TemplateClient::Class::_instance;

auto TemplateClient::loadConfig(const ConfigIntializer &initializer,
		utils::json::decoder::Object &jsonObject,
		config::Resolver &resolver,
		const FallbackConfigHandler &fallbackHandler) -> void
{
	// Get a reference that allows us to modify our own config attributes
    auto &&configAttributes = initializer[Class::instance().configHandle()];

	// Go through all the members of the JSON object that represents this object
	for (auto && [name, value] : jsonObject)
    {
		// TODO: load configuration parameters
		if (name == u8"TODO"sv)
		{
			// TODO: parse the value correctly
			auto todo = value.asNumber<std::uint64_t>();

			// TODO: check that the value is valid
			if (!"TODO")
			{
				// TODO: use an error message that tells the user exactly what is wrong
				utils::json::decoder::throwWithLocation(value, std::runtime_error("TODO is wrong with TODO parameter of template client"));
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
		utils::json::decoder::throwWithLocation(jsonObject, std::runtime_error("TODO is wrong with template client"));
	}
}

auto TemplateClient::performReconnectTask(const process::ExecutionContext &context) -> void
{
	// Only perform the reconnect if we are supposed to be connected in the first place
	if (_connectionRequestCount.load(std::memory_order_relaxed) == 0)
	{
		return;
	}
	// Also don't reconnect if we are already connected
	if (_handle)
	{
		return;
	}

	// TODO: check _lastError to see if a reconnect can succeed at all, and bail if it can't. A reconnect need not be attempted
	// if it requires non-existent hardware, like a missing network adapter or I/O card, for example.
	// see isConnectionError() for an example on how to check error codes.

	// Attempt a connection
	connect(context.scheduledTime());
}

auto TemplateClient::connect(std::chrono::system_clock::time_point timeStamp) -> void
{
	try
	{
		// TODO: try to establish the connection, and set the _handle object

		// TODO: if the connect function does not throw errors, but uses return types or internal handle state,
		// throw an std::system_error here on failure, or call updateState() directly.
		
		// Note: If your connect function uses normal system error codes (errno on Linux or GetLastError() on Windows), you
		// should create std::error_codes using std::system_category(). If you are using a library and/or protocol that provides
		// its own error codes, you should define a custom error category.

		// The connection was successful
		updateState(timeStamp, std::error_code());
	}
	// TODO: if your connection function throws exceptions that are not derived from std::system_error, but that
	// still provide some sort of error code, you should catch those exceptions separately and wrap the error code in a custom
	// error category.
	catch (const std::exception &exception)
	{
		// Get the error from the current exception using this special utility function
		const auto error = utils::eh::currentErrorCode();
		
		// Update the state
		updateState(timeStamp, error);
	}
}

auto TemplateClient::disconnect(std::chrono::system_clock::time_point timeStamp) -> void
{
	// Reset the handle in any case, even if we fail, because the connection state should be false after this
	auto handle = std::exchange(_handle, Handle());

	// TODO: close the connection, ignoring any errors. If the disconnect function can throw exceptions,
	// these shoudl be caucht and ignored.

	// This is always a graceful disconnect, regardless of what happened, so never include an error code.
	updateState(timeStamp, CustomError::NotConnected);
}

auto TemplateClient::updateState(std::chrono::system_clock::time_point timeStamp, std::error_code error, const ErrorSink *excludeErrorSink)
	-> void
{
	// First, check if anything changed
	if (error == _lastError)
	{
		return;
	}

	// Get the old and new state
	const auto wasConnected = !_lastError;
	const auto connected = !error;
	
	// Make a write sentinel
	memory::WriteSentinel sentinel { _stateDataBlock };
	auto &state = *sentinel;
	const auto &oldState = sentinel.oldValue();

	// Update the state
	state._connectionState = !error;

	// Update the change time, if necessary. We always need to write the change time, even if it is the same as before,
	// because the memory resource might use swap-in.
	state._connectionTime = wasConnected != connected ? timeStamp : oldState._connectionTime;

	// Update the error, using NotConnected as error for a graceful disconnect
	state._error = attributes::errorCode(error);

	// Commit the data before sending the events
	sentinel.commit();

	// Fire any events
	if (!wasConnected && connected)
	{
		_connectedEvent.fire();
	}
	else if (wasConnected && !connected)
	{
		_disconnectedEvent.fire();
	}

	// Notify all error sinks
	for (auto &&sink : _errorSinks)
	{
		if (&sink.get() != excludeErrorSink)
		{
			sink.get().clientStateChanged(timeStamp, error);
		}
	}
}

auto TemplateClient::isConnectionError(std::error_code error) noexcept -> bool
{
	// TODO: check if this error affects the connection as a whole, and bail if it doesn't.
	// This function should return true on errors that signal that the entire client has stopped working,
	// like timeouts and network errors, and false on errors thst only affect some inputs and/or outputs, such as
	// unknown input or output, type mismatch, range errors etc.

	// Example code suitable for socket errors:

	// Check system errors
	if (error.category() == std::system_category())
	{
		switch (error.value())
		{
	#ifdef _WIN32
		case WSAEBADF:
		case WSAENOTSOCK:
		case ERROR_INVALID_HANDLE:
		case WSAECONNRESET:
		case WSAECONNABORTED:
		case WSAENETRESET:
		case WSAESHUTDOWN:
		case WSAENETUNREACH:
		case WSAEHOSTUNREACH:
		case WSAEHOSTDOWN:
		case WSAENOTCONN:
		case ERROR_BROKEN_PIPE:
	#else // _WIN32
		case EBADF:
		case ECONNRESET:
		case ECONNABORTED:
		case ENETRESET:
		case ESHUTDOWN:
		case ENETUNREACH:
		case EHOSTUNREACH:
		case EHOSTDOWN:
		case ENOTCONN:
		case EPIPE:
	#endif // _WIN32
			return true;

		default:
			return false;
		}
	}

	// Check custom errors:
	if (error.category() == customErrorCategory())
	{
		switch (CustomError(error.value()))
		{
		case CustomError::NotConnected:
		case CustomError::UnknownError:
			// TODO: add case statements for other relevant custom errors (like e.g. timeout) here
			return true;

		case CustomError::NoError:
		default:
			return false;
		}
	}

	// No other categories need apply
	else
	{
		return false;
	}
}

auto TemplateClient::requestConnect(std::chrono::system_clock::time_point timeStamp) noexcept -> void
{
	// increment the count
	const auto oldCount = _connectionRequestCount++;

	// connect if the old count was 0
	if (oldCount == 0)
	{
		connect(timeStamp);
	}
}

auto TemplateClient::requestDisconnect(std::chrono::system_clock::time_point timeStamp) noexcept -> void
{
	// decrement the count
	const auto newCount = --_connectionRequestCount;

	// disconnect if the new count is 0
	if (newCount == 0)
	{
		disconnect(timeStamp);
	}
}

auto TemplateClient::handleError(std::chrono::system_clock::time_point timeStamp, std::error_code error, const ErrorSink *sender) noexcept -> void
{
	// Ignore any new errors if we already have an error (the first error always wins).
	if (_lastError)
	{
		return;
	}
	// Check if this error affects the connection as a whole, and bail if it doesn't. This function shoudl
	if (!isConnectionError(error))
	{
		return;
	}

	// Reset the handle
	// TODO: gracefully close the handle, if this is necessary
	_handle = Handle();

	// update the error state
	updateState(timeStamp, error, sender);
}

auto TemplateClient::createSubservice(const process::MicroserviceClass &ioClass, plugin::SharedFactory<process::Microservice> &factory)
	-> std::shared_ptr<process::Microservice>
{
	if (&ioClass == &TemplateTransaction::Class::instance())
	{
		return factory.makeShared<TemplateTransaction>(*this);
	}

	// TODO: add any other supported subservice types

	return nullptr;
}

auto TemplateClient::resolveAttribute(std::u16string_view name) -> const model::Attribute *
{
	// TODO: add any additional attributes this class supports
	return model::Attribute::resolve(name,
		attributes::kConnectionState,
		attributes::kConnectionTime,
		attributes::kError);
}

auto TemplateClient::resolveTask(std::u16string_view name) -> std::shared_ptr<process::Task>
{
	if (name == process::Task::kReconnect)
	{
		return std::shared_ptr<process::Task>(sharedFromThis(), &_reconnectTask);
	}

	// TODO resolve any additional tasks

	return nullptr;
}

auto TemplateClient::resolveEvent(std::u16string_view name) -> std::shared_ptr<process::Event>
{
	// Check all the events we support
	if (name == process::Event::kConnected)
	{
		return std::shared_ptr<process::Event>(sharedFromThis(), &_connectedEvent);
	}
	else if (name == process::Event::kDisconnected)
	{
		return std::shared_ptr<process::Event>(sharedFromThis(), &_disconnectedEvent);
	}

	// TODO resolve any additional tasks

	return nullptr;
}

auto TemplateClient::readHandle(const model::Attribute &attribute) const noexcept -> data::ReadHandle
{
	// Try our attributes
	if (attribute == attributes::kConnectionState)
	{
		return _stateDataBlock.member(&State::_connectionState);
	}
	else if (attribute == attributes::kConnectionTime)
	{
		return _stateDataBlock.member(&State::_connectionTime);
	}
	else if (attribute == attributes::kError)
	{
		return _stateDataBlock.member(&State::_error);
	}

	// TODO add support for any additional attributes

	// Nothing found
	return data::ReadHandle::Error::Unknown;
}

auto TemplateClient::realize() -> void
{
	// Create the data block
	_stateDataBlock.create(memory::memoryResources::data());
}

auto TemplateClient::ReconnectTask::preparePreOperational(const process::ExecutionContext &context) -> Status
{
	// Request a connection
	_target.get().requestConnect(context.scheduledTime());

	return Status::Ready;
}

auto TemplateClient::ReconnectTask::preOperational(const process::ExecutionContext &context) -> Status
{
	// Do the same as in operational
	operational(context);

	return Status::Ready;
}

auto TemplateClient::ReconnectTask::operational(const process::ExecutionContext &context) -> void
{
	_target.get().performReconnectTask(context);
}

auto TemplateClient::ReconnectTask::preparePostOperational(const process::ExecutionContext &context)
	-> Status
{
	// Request a disconnect
	_target.get().requestDisconnect(context.scheduledTime());

	return Status::Completed;
}

} // namespace xentara::plugins::templateUplink
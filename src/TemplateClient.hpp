// Copyright (c) embedded ocean GmbH
#pragma once

#include "Attributes.hpp"
#include "CustomError.hpp"

#include <xentara/memory/Array.hpp>
#include <xentara/memory/memoryResources.hpp>
#include <xentara/plugin/EnableSharedFromThis.hpp>
#include <xentara/process/Event.hpp>
#include <xentara/process/Microservice.hpp>
#include <xentara/process/MicroserviceClass.hpp>
#include <xentara/process/Task.hpp>
#include <xentara/utils/core/Uuid.hpp>
#include <xentara/utils/tools/Unique.hpp>

#include <string_view>
#include <functional>
#include <forward_list>

// TODO: rename namespace
namespace xentara::plugins::templateUplink
{

using namespace std::literals;

// A class representing a client for specific type of service that data can be sent to.
// TODO: rename this class to something more descriptive
class TemplateClient final : public process::Microservice, public plugin::EnableSharedFromThis<TemplateClient>
{
private:
	// A structure used to store the class specific attributes within an element's configuration
	struct Config final
	{
		// TODO: Add custom config attributes
	};
	
public:
	// The class object containing meta-information about this element type
	class Class final : public process::MicroserviceClass
	{
	public:
		// Gets the global object
		static auto instance() -> Class&
		{
			return _instance;
		}

	    // Returns the array handle for the class specific attributes within an element's configuration
	    auto configHandle() const -> const auto &
        {
            return _configHandle;
        }

		auto name() const -> std::u16string_view final
		{
			// TODO: change class name
			return u"TemplateClient"sv;
		}
	
		auto uuid() const -> utils::core::Uuid final
		{
			// TODO: assign a unique UUID
			return "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb"_uuid;
		}

	private:
	    // The array handle for the class specific attributes within an element's configuration
		memory::Array::ObjectHandle<Config> _configHandle { config().appendObject<Config>() };

		// The global object that represents the class
		static Class _instance;
	};

	// A handle used to access the client
	// TODO: implement a proper handle
	class Handle final : private utils::tools::Unique
	{
	public:
		// determines of the client is connected
		explicit operator bool() const noexcept
		{
			// TODO: return the actual state
			return false;
		}
	};

	// Interface for objects that want to be notified of errors
	class ErrorSink
	{
	public:
		// Interfaces should have a virtual destructor
		// (The = 0 ensures that this class will remain abstract, even if we should remove all other abstract functions later)
		virtual ~ErrorSink() = 0;

		// Called on error, or on success. This function is called in three instances:
		// 1. A connection was successfully established:	error will be a default costructed std::error_code object
		// 2. A connection was gracefully closed:			error will be a CustomError::NotConnected
		// 3. The connection was lost unexpectedly:			error will contain an appropriate error code
		virtual auto clientStateChanged(std::chrono::system_clock::time_point timeStamp, std::error_code error) -> void = 0;
	};

	// Adds an error sink
	auto addErrorSink(std::reference_wrapper<ErrorSink> sink)
	{
		_errorSinks.push_front(sink);
	}

	// Request that the client be connected. Must be balanced by a requestDisconnect.
	// If this is the first request, then the connection will be attempted, and the function will not return until
	// the connection has been successfully established, or has failed. In either case, error sinks will be notified,
	// so any error sinks calling this must be prepared to have clientStateChanged() called from within this function.
	auto requestConnect(std::chrono::system_clock::time_point timeStamp) noexcept -> void;

	// Request that the client be disconnected. Must balance a corresponding requestConnect.
	// If this is the last request, then the connection will be closed, and the function will not return until
	// the connection has been terminated. All error sinks will be notified with error code CustomError::NotConnected,
	// so any error sinks calling this must be prepared to have clientStateChanged() called from within this function.
	auto requestDisconnect(std::chrono::system_clock::time_point timeStamp) noexcept -> void;
	
	// Notifies the client that an error was detected from outside, e.g. when reading or writing an I/O point.
	// If this error affects the client as a whole, error sinks will be notified. If the sender is an error sink itself,
	// and does not which to be notified, but intends to handle the error itself instead, it can pass a pointer to itself as the sender parameter. 
	auto handleError(std::chrono::system_clock::time_point timeStamp, std::error_code error, const ErrorSink *sender = nullptr) noexcept -> void;

	// Checks whether the client is up
	auto connected() const -> bool
	{
		return bool(_handle);
	}

	// Returns a handle to the client
	auto handle() const -> const Handle &
	{
		return _handle;
	}

	auto createSubservice(const process::MicroserviceClass &microserviceClass, plugin::SharedFactory<process::Microservice> &factory) -> std::shared_ptr<process::Microservice> final;

	auto resolveAttribute(std::u16string_view name) -> const model::Attribute * final;
	
	auto resolveTask(std::u16string_view name) -> std::shared_ptr<process::Task> final;

	auto resolveEvent(std::u16string_view name) -> std::shared_ptr<process::Event> final;

	auto readHandle(const model::Attribute &attribute) const noexcept -> data::ReadHandle final;

	auto realize() -> void final;

protected:
	auto loadConfig(const ConfigIntializer &initializer,
		utils::json::decoder::Object &jsonObject,
		config::Resolver &resolver,
		const FallbackConfigHandler &fallbackHandler) -> void final;

private:
	// This structure represents the current state of the client
	struct State
	{
		// The state of the connection
		bool _connectionState { false };
		// The last time the component was connected or disconnected
		std::chrono::system_clock::time_point _connectionTime { std::chrono::system_clock::time_point::min() };
		// The error code when connecting, or 0 for none.
		attributes::ErrorCode _error { attributes::errorCode(CustomError::NotConnected) };
	};

	// This class providing callbacks for the Xentara scheduler for the "reconnect" task
	class ReconnectTask final : public process::Task
	{
	public:
		// This constuctor attached the task to its target
		ReconnectTask(std::reference_wrapper<TemplateClient> target) : _target(target)
		{
		}

		auto stages() const -> Stages final
		{
			return Stage::PreOperational | Stage::Operational | Stage::PostOperational;
		}

		auto preparePreOperational(const process::ExecutionContext &context) -> Status final;

		auto preOperational(const process::ExecutionContext &context) -> Status final;

		auto operational(const process::ExecutionContext &context) -> void final;

		auto preparePostOperational(const process::ExecutionContext &context) -> Status final;

	private:
		// A reference to the target element
		std::reference_wrapper<TemplateClient> _target;
	};
	
	// This function is called by the "reconnect" task. It attempts to reconnect any disconnected I/O components.
	auto performReconnectTask(const process::ExecutionContext &context) -> void;

	// Attempts to establish a connection to the client and updates the state accordingly. This function will notify error sinks if anything changes.
	auto connect(std::chrono::system_clock::time_point timeStamp) -> void;

	// Terminates the connection to the client and updates the state accordingly. This function will notify error sinks if anything changes.
	auto disconnect(std::chrono::system_clock::time_point timeStamp) -> void;

	// Updates the state and sends events
	auto updateState(std::chrono::system_clock::time_point timeStamp, std::error_code error, const ErrorSink *excludeErrorSink = nullptr) -> void;

	// Checks whether an error is the result of a lost connection
	static auto isConnectionError(std::error_code error) noexcept -> bool;

	// A Xentara event that is fired when the connection is established
	process::Event _connectedEvent;
	// A Xentara event that is fired when the connection is closed or lost
	process::Event _disconnectedEvent;

	// The "reconnect" task
	ReconnectTask _reconnectTask { *this };

	// A list of objects that want to be notified of errors
	std::forward_list<std::reference_wrapper<ErrorSink>> _errorSinks;

	// The number of people who would like this component to be connected
	std::atomic<std::size_t> _connectionRequestCount { 0 };

	// A handle to the client
	Handle _handle;
	// The last error we encountered.
	// - If the connection is open, this will be std::error_code()
	// - If the connection was closed gracefully, this will be CustomError::NotConnected;
	std::error_code _lastError { CustomError::NotConnected };

	// The data block that contains the state
	memory::ObjectBlock<memory::memoryResources::Data, State> _stateDataBlock;
};

inline TemplateClient::ErrorSink::~ErrorSink() = default;

} // namespace xentara::plugins::templateUplink
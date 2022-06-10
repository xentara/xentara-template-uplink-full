// Copyright (c) embedded ocean GmbH
#pragma once

#include "TemplateClient.hpp"
#include "TemplateRecord.hpp"
#include "CustomError.hpp"
#include "Attributes.hpp"

#include <xentara/memory/Array.hpp>
#include <xentara/memory/memoryResources.hpp>
#include <xentara/plugin/EnableSharedFromThis.hpp>
#include <xentara/process/Event.hpp>
#include <xentara/process/Microservice.hpp>
#include <xentara/process/MicroserviceClass.hpp>
#include <xentara/process/Task.hpp>
#include <xentara/utils/core/RawDataBlock.hpp>
#include <xentara/utils/core/Uuid.hpp>

#include <functional>
#include <string_view>
#include <forward_list>

// TODO: rename namespace
namespace xentara::plugins::templateUplink
{

using namespace std::literals;

// A class representing a data transaction for writing data to a client.
// TODO: rename this class to something more descriptive
class TemplateTransaction final : public process::Microservice, public TemplateClient::ErrorSink, public plugin::EnableSharedFromThis<TemplateTransaction>
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
			return u"TemplateTransaction"sv;
		}
	
		auto uuid() const -> utils::core::Uuid final
		{
			// TODO: assign a unique UUID
			return "cccccccc-cccc-cccc-cccc-cccccccccccc"_uuid;
		}

	private:
	    // The array handle for the class specific attributes within an element's configuration
		memory::Array::ObjectHandle<Config> _configHandle { config().appendObject<Config>() };

		// The global object that represents the class
		static Class _instance;
	};

	// This constructor attaches the output to its client
	TemplateTransaction(std::reference_wrapper<TemplateClient> client) :
		_client(client)
	{
		client.get().addErrorSink(*this);
	}

	auto resolveAttribute(std::u16string_view name) -> const model::Attribute * final;
	
	auto resolveTask(std::u16string_view name) -> std::shared_ptr<process::Task> final;

	auto resolveEvent(std::u16string_view name) -> std::shared_ptr<process::Event> final;

	auto readHandle(const model::Attribute &attribute) const noexcept -> data::ReadHandle final;

	auto realize() -> void final;

	auto prepare() -> void final;

	auto clientStateChanged(std::chrono::system_clock::time_point timeStamp, std::error_code error) -> void final;

protected:
	auto loadConfig(const ConfigIntializer &initializer,
		utils::json::decoder::Object &jsonObject,
		config::Resolver &resolver,
		const FallbackConfigHandler &fallbackHandler) -> void final;

private:
	// This structure represents the current state of the transaction
	struct State final
	{
		// Whether the transaction is being ztransmitted correctly
		bool _transactionState { false };
		// The last time the records were sent (successfully or not)
		std::chrono::system_clock::time_point _sendTime { std::chrono::system_clock::time_point::min() };
		// The error code when sending the records the value, or 0 for none.
		attributes::ErrorCode _error { attributes::errorCode(CustomError::NotConnected) };
	};

	// This class providing callbacks for the Xentara scheduler for the "collect" task
	class CollectTask final : public process::Task
	{
	public:
		// This constuctor attached the task to its target
		CollectTask(std::reference_wrapper<TemplateTransaction> target) : _target(target)
		{
		}

		auto stages() const -> Stages final
		{
			return Stage::PreOperational | Stage::Operational;
		}

		auto preparePreOperational(const process::ExecutionContext &context) -> Status final;

		auto preOperational(const process::ExecutionContext &context) -> Status final;

		auto operational(const process::ExecutionContext &context) -> void final;

	private:
		// A reference to the target element
		std::reference_wrapper<TemplateTransaction> _target;
	};

	// This class providing callbacks for the Xentara scheduler for the "send" task
	class SendTask final : public process::Task
	{
	public:
		// This constuctor attached the task to its target
		SendTask(std::reference_wrapper<TemplateTransaction> target) : _target(target)
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
		std::reference_wrapper<TemplateTransaction> _target;
	};

	// This function is forwarded to the client.
	auto requestConnect(std::chrono::system_clock::time_point timeStamp) noexcept -> void
	{
		_client.get().requestConnect(timeStamp);
	}

	// This function is forwarded to the client.
	auto requestDisconnect(std::chrono::system_clock::time_point timeStamp) noexcept -> void
	{
		_client.get().requestDisconnect(timeStamp);
	}

	// This function is called by the "collect" task. It collects data to be sent to the client.
	auto performCollectTask(const process::ExecutionContext &context) -> void;
	// Collects the data for all the records and appends it to the pending data
	auto collectData(std::chrono::system_clock::time_point timeStamp) -> void;

	// This function is called by the "send" task. It attempts to send the collected records if the client is up.
	auto performSendTask(const process::ExecutionContext &context) -> void;
	// Attempts to write send the collected records to the client and updates the state accordingly.
	auto send(std::chrono::system_clock::time_point timeStamp) -> void;	
	// Handles a send error
	auto handleSendError(std::chrono::system_clock::time_point timeStamp, std::error_code error) -> void;

	// Updates the state
	auto updateState(std::chrono::system_clock::time_point timeStamp, std::error_code error = std::error_code()) -> void;

	// The client this transaction belongs to
	std::reference_wrapper<TemplateClient> _client;
	// The records to be collected
	std::forward_list<TemplateRecord> _records;

	// The data to be sent
	// TODO: use a more appropriate type for buffering the data
	utils::core::RawDataBlock _pendingData;

	// A Xentara event that is fired when the records were successfully sent to the client
	process::Event _sentEvent;
	// A Xentara event that is fired when a send error occurred
	process::Event _sendErrorEvent;

	// The "collect" task
	CollectTask _collectTask { *this };
	// The "send" task
	SendTask _sendTask { *this };

	// The data block that contains the state
	memory::ObjectBlock<memory::memoryResources::Data, State> _stateDataBlock;
};

} // namespace xentara::plugins::templateUplink
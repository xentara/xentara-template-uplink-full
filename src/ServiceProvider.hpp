// Copyright (c) embedded ocean GmbH
#pragma once

#include "TemplateClient.hpp"
#include "TemplateTransaction.hpp"

#include <xentara/process/ServiceProvider.hpp>
#include <xentara/utils/core/Uuid.hpp>

#include <string_view>

// TODO: rename namespace
namespace xentara::plugins::templateUplink
{

using namespace std::literals;

// The driver class
class ServiceProvider final : public process::ServiceProvider
{
public:
	auto name() const -> std::u16string_view final
	{
		// TODO: change class name
		return u"TemplateUplink"sv;
	}

	auto uuid() const -> utils::core::Uuid final
	{
		// TODO: assign a unique UUID
		return "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"_uuid;
	}

	auto registerObjects(Registry &registry) -> void final
	{
		// TODO: register any additional element classes
		registry <<
			TemplateClient::Class::instance() <<
			TemplateTransaction::Class::instance();
	}

	auto createEnvironment() -> std::unique_ptr<process::ServiceProvider::Environment> final;

private:
	// The service provider runtime environment
	class Environment;
};

} // namespace xentara::plugins::templateUplink
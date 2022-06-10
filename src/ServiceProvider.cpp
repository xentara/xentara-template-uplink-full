// Copyright (c) embedded ocean GmbH
#include "ServiceProvider.hpp"

#include <xentara/plugin/SharedFactory.hpp>
#include <xentara/process/Microservice.hpp>

namespace xentara::plugins::templateUplink
{

class ServiceProvider::Environment : public process::ServiceProvider::Environment
{
public:
	// TODO: add constructor to perform global initialization, if necessary

	// TODO: add destructor to perform global cleanup, if necessary

	auto createMicroservice(const process::MicroserviceClass &componentClass, plugin::SharedFactory<process::Microservice> &factory)
		-> std::shared_ptr<process::Microservice> final;
};

auto ServiceProvider::Environment::createMicroservice(const process::MicroserviceClass &componentClass,
	plugin::SharedFactory<process::Microservice> &factory)
	-> std::shared_ptr<process::Microservice>
{
	if (&componentClass == &TemplateClient::Class::instance())
	{
		return factory.makeShared<TemplateClient>();
	}

	// TODO: handle any additional top-level microservice classes

	return nullptr;
}

auto ServiceProvider::createEnvironment() -> std::unique_ptr<process::ServiceProvider::Environment>
{
	return std::make_unique<Environment>();
}

} // namespace xentara::plugins::templateUplink
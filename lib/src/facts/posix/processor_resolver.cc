#include <facter/facts/posix/processor_resolver.hpp>
#include <facter/facts/collection.hpp>
#include <facter/facts/fact.hpp>
#include <facter/facts/scalar_value.hpp>
#include <facter/logging/logging.hpp>
#include <facter/execution/execution.hpp>
#include <cstring>

using namespace std;
using namespace facter::facts;
using namespace facter::execution;

LOG_DECLARE_NAMESPACE("facts.posix.processor");

namespace facter { namespace facts { namespace posix {

    processor_resolver::processor_resolver() :
        resolver(
            "processor",
            {
                fact::processor_count,
                fact::physical_processor_count,
                fact::hardware_isa,
                fact::hardware_model,
            },
            {
                string("^") + fact::processor + "[0-9]+$",
            })
    {
    }

    void processor_resolver::resolve_facts(collection& facts)
    {
        // Resolve the hardware related facts
        utsname name;
        memset(&name, 0, sizeof(name));
        if (uname(&name) != 0) {
            LOG_WARNING("uname failed: %1% (%2%): %3% and %4% facts are unavailable.", strerror(errno), errno, fact::hardware_isa, fact::hardware_model);
        } else {
            resolve_hardware_model(facts, name);
            resolve_hardware_isa(facts, name);
        }

        // Resolve the architecture
        resolve_architecture(facts);

        // Resolve the processors facts
        resolve_processors(facts);
    }

    void processor_resolver::resolve_hardware_isa(collection& facts, utsname const& name)
    {
        // The utsname struct doesn't have a member for "uname -p", so we need to execute
        auto result = execute("uname", { "-p" });
        if (!result.first || result.second.empty()) {
            return;
        }
        facts.add(fact::hardware_isa, make_value<string_value>(move(result.second)));
    }

    void processor_resolver::resolve_hardware_model(collection& facts, utsname const& name)
    {
        // There is a corresponding field for "uname -m", so use it
        string value = name.machine;
        if (value.empty()) {
            return;
        }
        facts.add(fact::hardware_model, make_value<string_value>(move(value)));
    }

    void processor_resolver::resolve_architecture(collection& facts)
    {
        // By default, use the hardware model
        auto model = facts.get<string_value>(fact::hardware_model, false);
        if (!model) {
            return;
        }
        facts.add(fact::architecture, make_value<string_value>(model->value()));
    }

}}}  // namespace facter::facts::posix

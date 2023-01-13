// Copyright 2018 Bloomberg Finance L.P
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <metricsconfig.h>

#include <env.h>

#include <buildboxcommonmetrics_metricsconfigurator.h>

#include <memory>

namespace recc {

std::shared_ptr<StatsDPublisherType> get_statsdpublisher_from_config()
{
    const auto config = buildboxcommon::buildboxcommonmetrics::
        MetricsConfigurator::createMetricsConfig(
            RECC_METRICS_FILE, RECC_METRICS_UDP_SERVER, RECC_ENABLE_METRICS);

    return buildboxcommon::buildboxcommonmetrics::MetricsConfigurator::
        createMetricsPublisherWithConfig<StatsDPublisherType>(config);
}

} // namespace recc

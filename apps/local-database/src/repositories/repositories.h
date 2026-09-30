#ifndef REPOSITORIES_H
#define REPOSITORIES_H

#include "repository_mesh_group.h"
#include "repository_sensor.h"
#include "repository_actuator.h"
#include "repository_node.h"
#include "repository_mesh_group_member.h"
#include "repository_uuid_whitelist.h"
#include <memory>

struct AppRepositories {
    std::shared_ptr<SensorReadingRepository> sensor;
    std::shared_ptr<ActuatorRepository> actuator;
    std::shared_ptr<MeshGroupMemberRepository> group_member;
    std::shared_ptr<MeshGroupRepository> group;
    std::shared_ptr<NodeRepository> node;
    std::shared_ptr<UUIDWhitelistRepository> whitelist;

    static std::shared_ptr<AppRepositories> create(sqlite3* db) {
        auto repos = std::make_shared<AppRepositories>();
        repos->sensor = std::make_shared<SensorReadingRepository>(db);
        repos->actuator = std::make_shared<ActuatorRepository>(db);
        repos->group_member = std::make_shared<MeshGroupMemberRepository>(db);
        repos->group = std::make_shared<MeshGroupRepository>(db);
        repos->node = std::make_shared<NodeRepository>(db);
        repos->whitelist = std::make_shared<UUIDWhitelistRepository>(db);
        return repos;
    }
};

#endif 
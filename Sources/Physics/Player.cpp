// SPDX-License-Identifier: MIT
// clang-format off: Jolt requires its umbrella header first.
#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include "Physics/Player.hpp"
// clang-format on

namespace TwentyThreeSeventeen::Physics
{
    namespace
    {
        constexpr JPH::BroadPhaseLayer kStaticLayer{ 0 };

        class StaticLayers final: public JPH::BroadPhaseLayerInterface
        {
        public:
            auto
            GetNumBroadPhaseLayers() const -> JPH::uint override
            {
                return 1;
            }

            auto
            GetBroadPhaseLayer(JPH::ObjectLayer) const
                -> JPH::BroadPhaseLayer override
            {
                return kStaticLayer;
            }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
            auto
            GetBroadPhaseLayerName(JPH::BroadPhaseLayer) const -> const
                char * override
            {
                return "Apartment";
            }
#endif
        };

        class StaticFilter final
            : public JPH::ObjectVsBroadPhaseLayerFilter
            , public JPH::ObjectLayerPairFilter
        {
        public:
            auto
            ShouldCollide(JPH::ObjectLayer, JPH::BroadPhaseLayer) const
                -> bool override
            {
                return true;
            }

            auto
            ShouldCollide(JPH::ObjectLayer, JPH::ObjectLayer) const
                -> bool override
            {
                return true;
            }
        };

        auto
        Vector(Position value) -> JPH::Vec3
        {
            return { value.x, value.y, value.z };
        }

        class JoltSession final
        {
        public:
            JoltSession()
            {
                JPH::RegisterDefaultAllocator();
                JPH::Factory::sInstance = new JPH::Factory();
                JPH::RegisterTypes();
            }

            ~JoltSession()
            {
                JPH::UnregisterTypes();
                delete JPH::Factory::sInstance;
                JPH::Factory::sInstance = nullptr;
            }
        };

        auto
        InitializeJolt() -> void
        {
            static const JoltSession session;
        }
    } // namespace

    struct Player::World
    {
        StaticLayers layers;
        StaticFilter filter;
        JPH::TempAllocatorImpl scratch{ 2 * 1024 * 1024 };
        JPH::PhysicsSystem physics;
        JPH::Ref<JPH::CharacterVirtual> character;
    };

    Player::Player(std::span<const Obstacle> obstacles, Position spawn)
        : spawn_(spawn)
    {
        InitializeJolt();
        world_ = std::make_unique<World>();
        world_->physics.Init(512,
                             0,
                             128,
                             128,
                             world_->layers,
                             world_->filter,
                             world_->filter);
        auto &bodies = world_->physics.GetBodyInterface();
        for (const auto &obstacle : obstacles)
        {
            const auto halfSize = Vector(obstacle.halfSize);
            const auto radius
                = JPH::min(JPH::cDefaultConvexRadius, halfSize.ReduceMin());
            const JPH::BodyCreationSettings body{ new JPH::BoxShape(halfSize,
                                                                    radius),
                                                  Vector(obstacle.center),
                                                  JPH::Quat::sIdentity(),
                                                  JPH::EMotionType::Static,
                                                  0 };
            const auto id
                = bodies.CreateAndAddBody(body, JPH::EActivation::DontActivate);
            bodies.SetUserData(id, obstacle.subject);
        }
        world_->physics.OptimizeBroadPhase();

        JPH::CharacterVirtualSettings settings;
        settings.mShape = new JPH::RotatedTranslatedShape(
            JPH::Vec3(0, 0.87f, 0),
            JPH::Quat::sIdentity(),
            new JPH::CapsuleShape(0.57f, 0.28f));
        settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -0.30f);
        world_->character = new JPH::CharacterVirtual(&settings,
                                                      Vector(spawn_),
                                                      JPH::Quat::sIdentity(),
                                                      &world_->physics);
    }

    Player::~Player() = default;

    auto
    Player::Move(float x, float z, float seconds) -> void
    {
        auto &character = *world_->character;
        const auto old = character.GetLinearVelocity();
        const auto gravity = world_->physics.GetGravity();
        const float vertical = character.IsSupported()
                                   ? -0.55f
                                   : old.GetY() + gravity.GetY() * seconds;
        character.SetLinearVelocity(JPH::Vec3(x, vertical, z));
        character.ExtendedUpdate(seconds,
                                 gravity,
                                 {},
                                 {},
                                 {},
                                 {},
                                 {},
                                 world_->scratch);
        if (character.GetPosition().GetY() < -4)
        {
            character.SetPosition(Vector(spawn_));
            character.SetLinearVelocity(JPH::Vec3::sZero());
        }
    }

    auto
    Player::Eye() const -> Position
    {
        const auto position = world_->character->GetPosition();
        return { position.GetX(), position.GetY() + 1.58f, position.GetZ() };
    }

    auto
    Player::LookAt(Position direction) const -> std::uint64_t
    {
        JPH::RayCastResult hit;
        const JPH::RRayCast ray{ Vector(Eye()), Vector(direction) * 2.7f };
        if (!world_->physics.GetNarrowPhaseQuery().CastRay(ray, hit))
            return 0;
        return world_->physics.GetBodyInterface().GetUserData(hit.mBodyID);
    }
} // namespace TwentyThreeSeventeen::Physics

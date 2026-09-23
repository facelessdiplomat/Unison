#include <arena/text_map.hpp>

#include <arena/components.hpp>

#include <unison/sim/body_definition.hpp>
#include <unison/sim/transform.hpp>

#include <entt/entity/registry.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <format>
#include <numbers>
#include <optional>
#include <vector>

namespace arena
{

namespace
{

constexpr float kMetresPerColumn = 0.5F;
constexpr float kMetresPerRow = 1.0F;
constexpr float kHalfWidth = static_cast<float>(kMapColumns) * kMetresPerColumn / 2.0F;
constexpr float kHalfDepth = static_cast<float>(kMapRows) * kMetresPerRow / 2.0F;
constexpr float kFloorTop = 0.5F;
constexpr float kWallTop = 2.0F;

using Map = std::array<std::string, kMapRows>;

struct Cell
{
    std::size_t column = 0;
    std::size_t row = 0;
};

std::optional<Cell> cellOf(const unison::Float3& position)
{
    const float column = std::floor((position.x + kHalfWidth) / kMetresPerColumn);
    const float row = std::floor((position.z + kHalfDepth) / kMetresPerRow);

    if (column < 0.0F || row < 0.0F || column >= static_cast<float>(kMapColumns) || row >= static_cast<float>(kMapRows))
    {
        return std::nullopt;
    }

    return Cell{static_cast<std::size_t>(column), static_cast<std::size_t>(row)};
}

unison::Float3 centreOf(std::size_t column, std::size_t row)
{
    return unison::Float3{-kHalfWidth + (static_cast<float>(column) + 0.5F) * kMetresPerColumn,
                          0.0F,
                          -kHalfDepth + (static_cast<float>(row) + 0.5F) * kMetresPerRow};
}

unison::Float3 unrotated(const unison::Quaternion& rotation, const unison::Float3& vector)
{
    const unison::Float3 axis{-rotation.x, -rotation.y, -rotation.z};
    const unison::Float3 twice{2.0F * (axis.y * vector.z - axis.z * vector.y),
                               2.0F * (axis.z * vector.x - axis.x * vector.z),
                               2.0F * (axis.x * vector.y - axis.y * vector.x)};

    return unison::Float3{vector.x + rotation.w * twice.x + (axis.y * twice.z - axis.z * twice.y),
                          vector.y + rotation.w * twice.y + (axis.z * twice.x - axis.x * twice.z),
                          vector.z + rotation.w * twice.z + (axis.x * twice.y - axis.y * twice.x)};
}

bool isUnder(const unison::sim::Transform& transform, const unison::sim::BodyDefinition& body, unison::Float3 point)
{
    const unison::Float3 offset{point.x - transform.position.x, 0.0F, point.z - transform.position.z};
    const unison::Float3 local = unrotated(transform.rotation, offset);

    return std::abs(local.x) <= body.halfExtents.x && std::abs(local.z) <= body.halfExtents.z;
}

char markOf(const unison::sim::Transform& transform, const unison::sim::BodyDefinition& body)
{
    if (body.motion != unison::sim::BodyMotion::Static)
    {
        return 'C';
    }

    return transform.position.y + body.halfExtents.y >= kWallTop ? '#' : '=';
}

void drawBodies(const unison::sim::Frame& frame, Map& map)
{
    for (const auto [entity, transform, body] :
         frame.registry.view<const unison::sim::Transform, const unison::sim::BodyDefinition>().each())
    {
        if (body.shape != unison::sim::BodyShape::Box || transform.position.y <= kFloorTop)
        {
            continue;
        }

        for (std::size_t row = 0; row < kMapRows; ++row)
        {
            for (std::size_t column = 0; column < kMapColumns; ++column)
            {
                if (isUnder(transform, body, centreOf(column, row)))
                {
                    map[row][column] = markOf(transform, body);
                }
            }
        }
    }
}

void drawShots(const unison::sim::Frame& frame, Map& map)
{
    for (const auto [entity, transform, projectile] :
         frame.registry.view<const unison::sim::Transform, const Projectile>().each())
    {
        if (const std::optional<Cell> cell = cellOf(transform.position))
        {
            map[cell->row][cell->column] = '*';
        }
    }
}

struct Heading
{
    char arrow = 'v';
    int columnStep = 0;
    int rowStep = 1;
};

constexpr float kQuarterTurn = std::numbers::pi_v<float> / 2.0F;
constexpr std::array<Heading, 4> kHeadingsByQuarterTurn{
    Heading{'v', 0, 1}, Heading{'>', 1, 0}, Heading{'^', 0, -1}, Heading{'<', -1, 0}};

Heading headingOf(float yaw)
{
    const float quarterTurns = std::floor(yaw / kQuarterTurn + 0.5F);
    const float quarterTurnsIntoTheTurn = quarterTurns - 4.0F * std::floor(quarterTurns / 4.0F);

    return kHeadingsByQuarterTurn[static_cast<std::size_t>(quarterTurnsIntoTheTurn)];
}

unison::Float3 aheadOf(const unison::Float3& position, const Heading& heading)
{
    return unison::Float3{position.x + static_cast<float>(heading.columnStep) * kMetresPerColumn,
                          position.y,
                          position.z + static_cast<float>(heading.rowStep) * kMetresPerRow};
}

void drawPlayers(const unison::sim::Frame& frame, Map& map)
{
    for (const auto [entity, transform, slot, state] :
         frame.registry.view<const unison::sim::Transform, const PlayerSlot, const CharacterState>().each())
    {
        if (frame.registry.all_of<RespawnTimer>(entity))
        {
            continue;
        }

        const Heading heading = headingOf(state.yaw);

        if (const std::optional<Cell> ahead = cellOf(aheadOf(transform.position, heading)))
        {
            map[ahead->row][ahead->column] = heading.arrow;
        }

        if (const std::optional<Cell> cell = cellOf(transform.position))
        {
            map[cell->row][cell->column] = static_cast<char>('0' + slot.slot);
        }
    }
}

std::string playerLinesOf(const unison::sim::Frame& frame)
{
    struct PlayerLine
    {
        std::uint8_t slot = 0;
        std::string text;
    };

    std::vector<PlayerLine> lines;

    for (const auto [entity, slot, health, score] :
         frame.registry.view<const PlayerSlot, const Health, const Score>().each())
    {
        const std::string standing = frame.registry.all_of<RespawnTimer>(entity)
                                         ? std::string{"waiting to come back"}
                                         : std::format("health {}", health.points);

        lines.push_back(
            PlayerLine{slot.slot, std::format("slot {}: {}, kills {}\n", slot.slot, standing, score.kills)});
    }

    std::ranges::sort(lines, {}, &PlayerLine::slot);

    std::string text;

    for (const PlayerLine& line : lines)
    {
        text += line.text;
    }

    return text;
}

}

std::string textMapOf(const unison::sim::Frame& frame)
{
    Map map;
    map.fill(std::string(kMapColumns, '.'));

    drawBodies(frame, map);
    drawShots(frame, map);
    drawPlayers(frame, map);

    std::string text;

    for (const std::string& row : map)
    {
        text += row;
        text += '\n';
    }

    return text + playerLinesOf(frame);
}

}

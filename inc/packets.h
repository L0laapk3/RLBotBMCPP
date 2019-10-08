#pragma once

#include "flatbuffercontainer.h"

namespace rlbot {
typedef FlatbufferContainer<rlbot::flat::GameTickPacket> GameTickPacket;
typedef FlatbufferContainer<rlbot::flat::BallPrediction> BallPrediction;
typedef FlatbufferContainer<rlbot::flat::FieldInfo> FieldInfo;
typedef FlatbufferContainer<rlbot::flat::FieldInfo> MatchInfo;
} // namespace rlbot

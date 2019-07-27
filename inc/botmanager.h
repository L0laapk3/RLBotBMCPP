#pragma once

#include "bot.h"

#include "server.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <windows.h>

struct BotInstance {
  Bot *bot;
  std::thread thread;
};

template <typename T> class BotManager {
private:
  std::map<int, BotInstance *> bots;
  std::mutex bots_mutex;

public:
  static void BotThread(Bot *bot) {
    float lasttime = 0;

    while (true) {
      ByteBuffer flatbufferData = Interface::UpdateLiveDataPacketFlatbuffer();

      // Don't try to read the packets when they are very small.
      if (flatbufferData.size > 4) {
        const rlbot::flat::GameTickPacket *gametickpacket =
            flatbuffers::GetRoot<rlbot::flat::GameTickPacket>(
                flatbufferData.ptr);

        // Only run the bot when we recieve a new packet.
        if (lasttime != gametickpacket->gameInfo()->secondsElapsed()) {
          ByteBuffer fieldInfoData = Interface::UpdateFieldInfoFlatbuffer();
          ByteBuffer ballPredictionData = Interface::GetBallPrediction();

          // The fieldinfo or ball prediction might not have been set up by the
          // framework yet.
          if (fieldInfoData.size > 4 && ballPredictionData.size > 4) {
            const rlbot::flat::FieldInfo *fieldinfo =
                flatbuffers::GetRoot<rlbot::flat::FieldInfo>(fieldInfoData.ptr);
            const rlbot::flat::BallPrediction *ballprediction =
                flatbuffers::GetRoot<rlbot::flat::BallPrediction>(
                    ballPredictionData.ptr);

            int status = Interface::SetBotInput(
                bot->GetOutput(gametickpacket, fieldinfo, ballprediction),
                bot->index);

            Interface::Free(fieldInfoData.ptr);
            Interface::Free(ballPredictionData.ptr);
          }

          lasttime = gametickpacket->gameInfo()->secondsElapsed();
        } else {
          Sleep(1);
        }
      } else {
        Sleep(100);
      }

      Interface::Free(flatbufferData.ptr);
    }
  }

  void RunSingleBot(int index, int team, std::string name) {
    BotThread(InstantiateBot(index, team, name));
  }

  void StartBotServer(uint16_t port) {
    using namespace std::placeholders;

    std::thread tcpserver(Server::Run, port,
                          std::bind(&BotManager::RecieveMessage, this, _1));
    tcpserver.join();
  }

  void RecieveMessage(Message message) {
    if (message.command == Command::Add) {
      AddBot(message.index, message.team, message.name);
    } else if (message.command == Command::Remove) {
      RemoveBot(message.index);
    }
  }

  void AddBot(int index, int team, std::string name) {
    bots_mutex.lock();

    if (bots.find(index) == bots.end()) {
      BotInstance *instance = new BotInstance();
      instance->bot = InstantiateBot(index, team, name);
      instance->thread = std::thread(BotManager::BotThread, instance->bot);

      bots.insert(std::make_pair(index, instance));
    }

    bots_mutex.unlock();
  }

  void RemoveBot(int index) {
    bots_mutex.lock();

    delete bots[index]->bot;
    delete bots[index];

    bots.erase(index);

    bots_mutex.unlock();
  }

  Bot *InstantiateBot(int index, int team, std::string name) {
    return new T(index, team, name);
  }
};

// Copyright 2016 Google Inc. All rights reserved.
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

#include "firebase/app.h"
#include "firebase/messaging.h"
#include "firebase/util.h"

// Thin OS abstraction layer.
#include "main.h"  // NOLINT

// Execute all methods of the C++ Firebase Cloud Messaging API.
extern "C" int common_main(int argc, const char* argv[]) {
  ::firebase::App* app;
  ::firebase::messaging::PollableListener listener;

#if defined(__ANDROID__)
  app = ::firebase::App::Create(GetJniEnv(), GetActivity());
#else
  app = ::firebase::App::Create();
#endif  // defined(__ANDROID__)

  LogMessage("Initialized Firebase App.");

  LogMessage("Initialize the Messaging library");

  ::firebase::ModuleInitializer initializer;
  initializer.Initialize(
      app, &listener, [](::firebase::App* app, void* userdata) {
        LogMessage("Try to initialize Firebase Messaging");
        ::firebase::messaging::PollableListener* listener =
            static_cast<::firebase::messaging::PollableListener*>(userdata);
        return ::firebase::messaging::Initialize(*app, listener);
      });

  while (initializer.InitializeLastResult().status() !=
         firebase::kFutureStatusComplete) {
    if (ProcessEvents(100)) return 1;  // exit if requested
  }
  if (initializer.InitializeLastResult().error() != 0) {
    LogMessage("Failed to initialize Firebase Messaging: %s",
               initializer.InitializeLastResult().error_message());
    ProcessEvents(2000);
    return 1;
  }

  LogMessage("Initialized Firebase Cloud Messaging.");

  ::firebase::messaging::Subscribe("TestTopic");
  LogMessage("Subscribed to TestTopic");

  bool done = false;
  while (!done) {
    std::string token;
    if (listener.PollRegistrationToken(&token)) {
      LogMessage("Recieved Registration Token: %s", token.c_str());
    }

    ::firebase::messaging::Message message;
    while (listener.PollMessage(&message)) {
      LogMessage("Recieved a new message");
      LogMessage("This message was %s by the user",
                 message.notification_opened ? "opened" : "not opened");
      if (!message.from.empty()) LogMessage("from: %s", message.from.c_str());
      if (!message.error.empty())
        LogMessage("error: %s", message.error.c_str());
      if (!message.message_id.empty()) {
        LogMessage("message_id: %s", message.message_id.c_str());
      }
      if (!message.data.empty()) {
        LogMessage("data:");
        for (const auto& field : message.data) {
          LogMessage("  %s: %s", field.first.c_str(), field.second.c_str());
        }
      }
      if (message.notification) {
        LogMessage("notification:");
        if (!message.notification->title.empty()) {
          LogMessage("  title: %s", message.notification->title.c_str());
        }
        if (!message.notification->body.empty()) {
          LogMessage("  body: %s", message.notification->body.c_str());
        }
        if (!message.notification->icon.empty()) {
          LogMessage("  icon: %s", message.notification->icon.c_str());
        }
        if (!message.notification->tag.empty()) {
          LogMessage("  tag: %s", message.notification->tag.c_str());
        }
        if (!message.notification->color.empty()) {
          LogMessage("  color: %s", message.notification->color.c_str());
        }
        if (!message.notification->sound.empty()) {
          LogMessage("  sound: %s", message.notification->sound.c_str());
        }
        if (!message.notification->click_action.empty()) {
          LogMessage("  click_action: %s",
                     message.notification->click_action.c_str());
        }
      }
    }
    // Process events so that the client doesn't hang.
    done = ProcessEvents(1000);
  }

  ::firebase::messaging::Terminate();
  delete app;

  return 0;
}

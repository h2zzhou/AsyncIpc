// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IPC_IPC_CHANNEL_WIN_H_
#define IPC_IPC_CHANNEL_WIN_H_

#include "ipc/ipc_channel.h"

#include <queue>
#include <string>

#include "ipc/ipc_common.h"
#include "ipc/ipc_thread.h"
#include "ipc/ipc_channel_reader.h"

namespace IPC {

class Channel::ChannelImpl : public internal::ChannelReader,
                             public Thread::IOHandler {
 public:
  // Mirror methods of Channel, see ipc_channel.h for description.
  ChannelImpl(const IPC::ChannelHandle &channel_handle, Mode mode,
	  Listener* listener, Thread* thread);
  ~ChannelImpl();
  bool Connect();
  void Close();
  bool Send(Message* message);
  static bool IsNamedServerInitialized(const std::string& channel_id);
  DWORD peer_pid() const { return peer_pid_; }

 private:
  // ChannelReader implementation.
  virtual ReadState ReadData(char* buffer,
                             int buffer_len,
                             int* bytes_read) override;
  virtual bool WillDispatchInputMessage(Message* msg) override;
  bool DidEmptyInputBuffers() override;
  virtual void HandleHelloMessage(Message* msg) override;

  static const std::wstring PipeName(const std::string& channel_id,
                                 int32* secret);
  bool CreatePipe(const IPC::ChannelHandle &channel_handle, Mode mode);

  bool ProcessConnection();
  bool ProcessOutgoingMessages(Thread::IOContext* context,
                               DWORD bytes_written);

  // MessageLoop::IOHandler implementation.
  virtual void OnIOCompleted(Thread::IOContext* context,
                             DWORD bytes_transfered,
                             DWORD error);

 private:
  struct State {
    explicit State(ChannelImpl* channel);
    ~State();
	Thread::IOContext context;
    bool is_pending;
  };

  State input_state_;
  State output_state_;

  HANDLE pipe_;

  DWORD peer_pid_;

  // Messages to be sent are queued here.
  std::queue<Message*> output_queue_;

  // In server-mode, we have to wait for the client to connect before we
  // can begin reading.  We make use of the input_state_ when performing
  // the connect operation in overlapped mode.
  bool waiting_connect_;

  // This flag is set when processing incoming messages.  It is used to
  // avoid recursing through ProcessIncomingMessages, which could cause
  // problems.  TODO(darin): make this unnecessary
  bool processing_incoming_;

  // Determines if we should validate a client's secret on connection.
  bool validate_client_;

  // This is a unique per-channel value used to authenticate the client end of
  // a connection. If the value is non-zero, the client passes it in the hello
  // and the host validates. (We don't send the zero value fto preserve IPC
  // compatability with existing clients that don't validate the channel.)
  int32 client_secret_;

  Thread* thread_;

  DISALLOW_COPY_AND_ASSIGN(ChannelImpl);
};

}  // namespace IPC

#endif  // IPC_IPC_CHANNEL_WIN_H_

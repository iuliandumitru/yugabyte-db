// Copyright (c) YugaByte, Inc.

#include <memory>
#include <string>
#include <vector>

#include "yb/gutil/strings/substitute.h"
#include "yb/integration-tests/redis_table-test.h"
#include "yb/redisserver/redis_server.h"
#include "yb/rpc/redis_encoding.h"
#include "yb/util/cast.h"
#include "yb/util/test_util.h"

namespace yb {
namespace redisserver {

using std::string;
using std::unique_ptr;
using std::vector;
using strings::Substitute;
using yb::tablet::RedisTableTest;
using yb::rpc::EncodeAsArrays;
using yb::rpc::EncodeAsBulkString;
using yb::rpc::EncodeAsSimpleString;

class TestRedisService : public RedisTableTest {
 public:
  void SetUp() override;
  void TearDown() override;

  void SendCommandAndExpectTimeout(const string& cmd);

  void SendCommandAndExpectResponse(const string& cmd, const string& resp);

 private:
  Status SendCommandAndGetResponse(
      const string& cmd, int expected_resp_length, int timeout_in_millis = 1000);

  Socket client_sock_;
  unique_ptr<RedisServer> server_;
  int redis_server_port_ = 0;
  unique_ptr<FileLock> redis_port_lock_;
  static constexpr size_t kBufLen = 1024;
  uint8_t resp_[kBufLen];
};

void TestRedisService::SetUp() {
  RedisTableTest::SetUp();

  redis_server_port_ = GetFreePort(&redis_port_lock_);
  RedisServerOptions opts;
  opts.rpc_opts.rpc_bind_addresses = strings::Substitute("0.0.0.0:$0", redis_server_port_);

  auto master_rpc_addrs = master_rpc_addresses_as_strings();
  opts.master_addresses_flag = JoinStrings(master_rpc_addrs, ",");

  server_.reset(new RedisServer(opts));
  LOG(INFO) << "Initializing redis server...";
  CHECK_OK(server_->Init());

  LOG(INFO) << "Starting redis server...";
  CHECK_OK(server_->Start());
  LOG(INFO) << "Redis server successfully started.";

  Sockaddr remote;
  remote.ParseString("0.0.0.0", redis_server_port_);
  CHECK_OK(client_sock_.Init(0));
  CHECK_OK(client_sock_.SetNoDelay(false));
  LOG(INFO) << "Connecting to " << remote.ToString();
  CHECK_OK(client_sock_.Connect(remote));
}

void TestRedisService::TearDown() {
  EXPECT_OK(client_sock_.Close());
  RedisTableTest::TearDown();
}

Status TestRedisService::SendCommandAndGetResponse(
    const string& cmd, int expected_resp_length, int timeout_in_millis) {
  // Send the command.
  int32_t bytes_written = 0;
  EXPECT_OK(client_sock_.Write(util::to_uchar_ptr(cmd.c_str()), cmd.length(), &bytes_written));

  EXPECT_EQ(cmd.length(), bytes_written);

  // Receive the response.
  MonoTime deadline = MonoTime::Now(MonoTime::FINE);
  deadline.AddDelta(MonoDelta::FromMilliseconds(timeout_in_millis));
  size_t bytes_read = 0;
  RETURN_NOT_OK(client_sock_.BlockingRecv(resp_, expected_resp_length, &bytes_read, deadline));
  if (expected_resp_length != bytes_read) {
    return STATUS(
        IOError, Substitute("Received $1 bytes instead of $2", bytes_read, expected_resp_length));
  }
  return Status::OK();
}

void TestRedisService::SendCommandAndExpectTimeout(const string& cmd) {
  // Don't expect to receive even 1 byte.
  ASSERT_TRUE(SendCommandAndGetResponse(cmd, 1).IsTimedOut());
}

void TestRedisService::SendCommandAndExpectResponse(const string& cmd, const string& resp) {
  CHECK_OK(SendCommandAndGetResponse(cmd, resp.length()));

  // Verify that the response is as expected.
  CHECK_EQ(resp, string(reinterpret_cast<char*>(resp_), resp.length()));
}

TEST_F(TestRedisService, SimpleCommandInline) {
  SendCommandAndExpectResponse("TEST\r\n", "+OK\r\n");
}

TEST_F(TestRedisService, SimpleCommandMulti) {
  SendCommandAndExpectResponse("*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$4\r\nTEST\r\n", "+OK\r\n");
}

TEST_F(TestRedisService, BatchedCommandsInline) {
  SendCommandAndExpectResponse("TEST1\r\nTEST2\r\nTEST3\r\nTEST4\r\n",
                               "+OK\r\n+OK\r\n+OK\r\n+OK\r\n");
}

TEST_F(TestRedisService, BatchedCommandMulti) {
  SendCommandAndExpectResponse("*3\r\n$4\r\nset1\r\n$3\r\nfoo\r\n$4\r\nTEST\r\n"
                                   "*3\r\n$4\r\nset2\r\n$3\r\nfoo\r\n$4\r\nTEST\r\n"
                                   "*3\r\n$4\r\nset3\r\n$3\r\nfoo\r\n$4\r\nTEST\r\n",
                               "+OK\r\n+OK\r\n+OK\r\n");
}

TEST_F(TestRedisService, IncompleteCommandInline) {
  SendCommandAndExpectTimeout("TEST");
}

TEST_F(TestRedisService, IncompleteCommandMulti) {
  SendCommandAndExpectTimeout("*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$4\r\nTE");
}

TEST_F(TestRedisService, Echo) {
  SendCommandAndExpectResponse("*2\r\n$4\r\necho\r\n$3\r\nfoo\r\n", "+foo\r\n");
  SendCommandAndExpectResponse("*2\r\n$4\r\necho\r\n$8\r\nfoo bar \r\n", "+foo bar \r\n");
  SendCommandAndExpectResponse(
      EncodeAsArrays({  // The request is sent as a multi bulk array.
                         EncodeAsBulkString("echo"),
                         EncodeAsBulkString("foo bar")
                     }),
      EncodeAsSimpleString("foo bar")  // The response is in the simple string format.
      );
}

TEST_F(TestRedisService, TestSetOnly) {
  SendCommandAndExpectResponse("*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$4\r\nTEST\r\n", "+OK\r\n");
  SendCommandAndExpectResponse("*3\r\n$3\r\nset\r\n$4\r\nfool\r\n$4\r\nBEST\r\n", "+OK\r\n");
}

TEST_F(TestRedisService, DISABLED_TestSetThenGet) {
  SendCommandAndExpectResponse("*3\r\n$3\r\nset\r\n$3\r\nfoo\r\n$4\r\nTEST\r\n", "+OK\r\n");
  SendCommandAndExpectResponse("*2\r\n$3\r\nget\r\n$3\r\nfoo\r\n", "+TEST\r\n");
}
}  // namespace redisserver
}  // namespace yb
// Copyright 2025 John Manferdelli, All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
// or in the the file LICENSE-2.0.txt in the top level sourcedirectory
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License
// File: acl_rpc.cc

#include "certifier.pb.h"
#include "certifier.h"
#include "support.h"
#include "acl_rpc.h"
#include "acl.pb.h"


using namespace certifier::framework;

namespace certifier {
namespace acl_lib {


// For testing only
// The simulated channel makes the code ugly.
#ifdef TEST_SIMULATED_CHANNEL
const int max_size_buf = 4096;
int       size_buf = 0;
byte      simulated_buf[max_size_buf];

int simulated_sized_buf_read(string *out) {
  out->assign((char *)simulated_buf, size_buf);
  int t = size_buf;
  size_buf = 0;
  return t;
}

int simulated_buf_write(int n, byte *b) {
  if (n > max_size_buf)
    return -1;
  memcpy(simulated_buf, b, n);
  size_buf = n;
  return n;
}
#endif

// This makes testing easier
#ifdef TEST_SIMULATED_CHANNEL
extern acl_server_dispatch g_server;
#endif

int channel_read(SSL *channel, string *out) {

  int bytes_read;
#ifndef TEST_SIMULATED_CHANNEL
  bytes_read = sized_ssl_read(channel, out);
#else
  bytes_read = simulated_sized_buf_read(out);
#endif
  return bytes_read;
}

int channel_write(SSL *channel, int n, byte *buf) {
  int bytes_written;
#ifndef TEST_SIMULATED_CHANNEL
  bytes_written = sized_ssl_write(channel, n, buf);
#else
  bytes_written = simulated_buf_write(n, buf);
#endif
  return bytes_written;
}

// Functions supported
string authenticate_me_tag("authenticate_me");
string verify_me_tag("verify_me");
string open_resource_tag("open_resource");
string close_resource_tag("close_resource");
string read_resource_tag("read_resource");
string write_resource_tag("write_resource");
string add_access_right_tag("add_access_right");
string add_principal_tag("add_principal");
string delete_principal_tag("delete_principal");
string create_resource_tag("create_resource");
string delete_resource_tag("delete_resource");

acl_client_dispatch::acl_client_dispatch(SSL *channel) {
  channel_descriptor_ = channel;
  initialized_ = true;
}

acl_client_dispatch::~acl_client_dispatch() {}

bool acl_client_dispatch::rpc_authenticate_me(const string &principal_name,
                                              const string &serialized_creds,
                                              string       *output) {
  string   decode_parameters_str;
  string   encode_parameters_str;
  rpc_call input_call_struct;
  rpc_call output_call_struct;
  int      bytes_read = 0;

  // format input buffer, serialize it
  input_call_struct.set_function_name(authenticate_me_tag);
  string *in = input_call_struct.add_str_inputs();
  *in = principal_name;
  string *supplied_creds = input_call_struct.add_buf_inputs();
  supplied_creds->assign(serialized_creds.data(), serialized_creds.size());

  if (!input_call_struct.SerializeToString(&encode_parameters_str)) {
    printf("%s() error, line %d: Can't input\n", __func__, __LINE__);
    return false;
  }

  if (channel_write(channel_descriptor_,
                    encode_parameters_str.size(),
                    (byte *)encode_parameters_str.data())
      < 0) {
    printf("%s() error, line %d: Can't write to channel\n", __func__, __LINE__);
    return false;
  }
#ifdef TEST_SIMULATED_CHANNEL
  g_server.service_request();
#endif
  bytes_read = channel_read(channel_descriptor_, &decode_parameters_str);
  if (bytes_read < 0) {
    printf("%s() error, line %d: Can't read from channel\n",
           __func__,
           __LINE__);
    return false;
  }

  if (!output_call_struct.ParseFromString(decode_parameters_str)) {
    printf("%s() error, line %d: Can't parse return buffer\n",
           __func__,
           __LINE__);
    return false;
  }
  if (output_call_struct.function_name() != authenticate_me_tag) {
    printf("%s() error, line %d: wrong function name tag %s\n",
           __func__,
           __LINE__,
           output_call_struct.function_name().c_str());
    return false;
  }
  bool ret = output_call_struct.status();
  if (!ret) {
    printf("%s() error, line %d: output call status is false\n",
           __func__,
           __LINE__);
    return false;
  }
  if (output_call_struct.buf_outputs_size() < 1) {
    printf("%s() error, line %d: missing return argument\n",
           __func__,
           __LINE__);
    return false;
  }
  const string &out_nonce = output_call_struct.buf_outputs(0);
  output->assign(out_nonce.data(), out_nonce.size());
  return true;
}

bool acl_client_dispatch::rpc_verify_me(const string &principal_name,
                                        const string &signed_nonce) {
  string   decode_parameters_str;
  string   encode_parameters_str;
  rpc_call input_call_struct;
  rpc_call output_call_struct;
  int      bytes_read = 0;

  // format input buffer, serialize it
  input_call_struct.set_function_name(verify_me_tag);
  string *in1 = input_call_struct.add_str_inputs();
  *in1 = principal_name;
  string *in2 = input_call_struct.add_buf_inputs();
  *in2 = signed_nonce;

  if (!input_call_struct.SerializeToString(&encode_parameters_str)) {
    printf("%s() error, line %d: Can't input\n", __func__, __LINE__);
    return false;
  }

  if (channel_write(channel_descriptor_,
                    encode_parameters_str.size(),
                    (byte *)encode_parameters_str.data())
      < 0) {
    printf("%s() error, line %d: Can't write to channel\n", __func__, __LINE__);
    return false;
  }
#ifdef TEST_SIMULATED_CHANNEL
  g_server.service_request();
#endif
  bytes_read = channel_read(channel_descriptor_, &decode_parameters_str);
  if (bytes_read < 0) {
    printf("%s() error, line %d: Can't read from channel\n",
           __func__,
           __LINE__);
    return false;
  }

  if (!output_call_struct.ParseFromString(decode_parameters_str)) {
    printf("%s() error, line %d: Can't parse return buffer\n",
           __func__,
           __LINE__);
    return false;
  }
  if (!output_call_struct.has_function_name()) {
    printf("%s() error, line %d: No function name\n", __func__, __LINE__);
    return false;
  }
  if (output_call_struct.function_name() != verify_me_tag) {
    printf("%s() error, line %d: wrong function name tag %s\n",
           __func__,
           __LINE__,
           output_call_struct.function_name().c_str());
    return false;
  }
  bool ret = output_call_struct.status();
  if (!ret) {
    printf("%s() error, line %d: return status is false\n", __func__, __LINE__);
    return false;
  }
  return true;
}

bool acl_client_dispatch::rpc_open_resource(const string &resource_name,
                                            const string &access_right,
                                            int          *local_descriptor) {
  string   decode_parameters_str;
  string   encode_parameters_str;
  rpc_call input_call_struct;
  rpc_call output_call_struct;
  int      bytes_read = 0;

  // format input buffer, serialize it
  input_call_struct.set_function_name(open_resource_tag);
  string *pr_name = input_call_struct.add_str_inputs();
  *pr_name = resource_name;
  string *pr_access = input_call_struct.add_str_inputs();
  *pr_access = access_right;

  if (!input_call_struct.SerializeToString(&encode_parameters_str)) {
    printf("%s() error, line %d: Can't input\n", __func__, __LINE__);
    return false;
  }

  if (channel_write(channel_descriptor_,
                    encode_parameters_str.size(),
                    (byte *)encode_parameters_str.data())
      < 0) {
    printf("%s() error, line %d: Can't write to channel\n", __func__, __LINE__);
    return false;
  }
#ifdef TEST_SIMULATED_CHANNEL
  g_server.service_request();
#endif
  bytes_read = channel_read(channel_descriptor_, &decode_parameters_str);
  if (bytes_read < 0) {
    printf("%s() error, line %d: Can't read from channel\n",
           __func__,
           __LINE__);
    return false;
  }

  if (!output_call_struct.ParseFromString(decode_parameters_str)) {
    printf("%s() error, line %d: Can't parse return buffer\n",
           __func__,
           __LINE__);
    return false;
  }
  if (output_call_struct.function_name() != open_resource_tag) {
    printf("%s() error, line %d: wrong function name tag\n",
           __func__,
           __LINE__);
    return false;
  }
  if (output_call_struct.status() != true) {
    return false;
  }
  *local_descriptor = output_call_struct.int_outputs(0);
  return output_call_struct.status();
}

bool acl_client_dispatch::rpc_read_resource(const string &resource_name,
                                            int           local_descriptor,
                                            int           num_bytes,
                                            string       *bytes_output) {
  string   decode_parameters_str;
  string   encode_parameters_str;
  rpc_call input_call_struct;
  rpc_call output_call_struct;
  int      bytes_read = 0;

  // format input buffer, serialize it
  input_call_struct.set_function_name(read_resource_tag);
  input_call_struct.add_int_inputs((::int32_t)local_descriptor);
  input_call_struct.add_int_inputs((::int32_t)num_bytes);
  string *pr_str = input_call_struct.add_str_inputs();
  *pr_str = resource_name;
  if (!input_call_struct.SerializeToString(&encode_parameters_str)) {
    printf("%s() error, line %d: Can't input\n", __func__, __LINE__);
    return false;
  }

  if (channel_write(channel_descriptor_,
                    encode_parameters_str.size(),
                    (byte *)encode_parameters_str.data())
      < 0) {
    printf("%s() error, line %d: Can't write to channel\n", __func__, __LINE__);
    return false;
  }
#ifdef TEST_SIMULATED_CHANNEL
  g_server.service_request();
#endif
  bytes_read = channel_read(channel_descriptor_, &decode_parameters_str);
  if (bytes_read < 0) {
    printf("%s() error, line %d: Can't read from channel\n",
           __func__,
           __LINE__);
    return false;
  }

  if (!output_call_struct.ParseFromString(decode_parameters_str)) {
    printf("%s() error, line %d: Can't parse return buffer\n",
           __func__,
           __LINE__);
    return false;
  }
  if (!output_call_struct.has_function_name()) {
    printf("%s() error, line %d: has no function name tag\n",
           __func__,
           __LINE__);
    return false;
  }
  if (output_call_struct.function_name() != read_resource_tag) {
    printf("%s() error, line %d: wrong function name tag %s\n",
           __func__,
           __LINE__,
           output_call_struct.function_name().c_str());
    return false;
  }
  bool ret = output_call_struct.status();
  if (!ret) {
    return false;
  }
  if (output_call_struct.buf_outputs_size() < 1) {
    printf("%s() error, line %d: too few returned bufs\n", __func__, __LINE__);
    return false;
  }
  *bytes_output = output_call_struct.buf_outputs(0);
  return true;
}

bool acl_client_dispatch::rpc_write_resource(const string &resource_name,
                                             int           local_descriptor,
                                             const string &bytes_to_write) {

  string   decode_parameters_str;
  string   encode_parameters_str;
  rpc_call input_call_struct;
  rpc_call output_call_struct;
  int      bytes_read = 0;

  // format input buffer, serialize it
  input_call_struct.set_function_name(write_resource_tag);
  string *p_res = input_call_struct.add_str_inputs();
  *p_res = resource_name;
  string *buf_to_write = input_call_struct.add_buf_inputs();
  *buf_to_write = bytes_to_write;
  input_call_struct.add_int_inputs((::int32_t)local_descriptor);
  input_call_struct.add_int_inputs((::int32_t)bytes_to_write.size());

  if (!input_call_struct.SerializeToString(&encode_parameters_str)) {
    printf("%s() error, line %d: Can't input\n", __func__, __LINE__);
    return false;
  }

  if (channel_write(channel_descriptor_,
                    encode_parameters_str.size(),
                    (byte *)encode_parameters_str.data())
      < 0) {
    printf("%s() error, line %d: Can't write to channel\n", __func__, __LINE__);
    return false;
  }
#ifdef TEST_SIMULATED_CHANNEL
  g_server.service_request();
#endif
  bytes_read = channel_read(channel_descriptor_, &decode_parameters_str);
  if (bytes_read < 0) {
    printf("%s() error, line %d: Can't read from channel\n",
           __func__,
           __LINE__);
    return false;
  }

  if (!output_call_struct.ParseFromString(decode_parameters_str)) {
    printf("%s() error, line %d: Can't parse return buffer\n",
           __func__,
           __LINE__);
    return false;
  }
  if (output_call_struct.function_name() != write_resource_tag) {
    printf("%s() error, line %d: wrong function name tag\n",
           __func__,
           __LINE__);
    return false;
  }
  bool ret = output_call_struct.status();
  if (!ret) {
    return false;
  }
  return true;
}

bool acl_client_dispatch::rpc_close_resource(const string &resource_name,
                                             int           local_descriptor) {
  string   decode_parameters_str;
  string   encode_parameters_str;
  rpc_call input_call_struct;
  rpc_call output_call_struct;
  int      bytes_read = 0;

  // format input buffer, serialize it
  input_call_struct.set_function_name(close_resource_tag);
  string *in = input_call_struct.add_str_inputs();
  *in = resource_name;
  input_call_struct.add_int_inputs((::int32_t)local_descriptor);

  if (!input_call_struct.SerializeToString(&encode_parameters_str)) {
    printf("%s() error, line %d: Can't input\n", __func__, __LINE__);
    return false;
  }

  if (channel_write(channel_descriptor_,
                    encode_parameters_str.size(),
                    (byte *)encode_parameters_str.data())
      < 0) {
    printf("%s() error, line %d: Can't write to channel\n", __func__, __LINE__);
    return false;
  }
#ifdef TEST_SIMULATED_CHANNEL
  g_server.service_request();
#endif
  bytes_read = channel_read(channel_descriptor_, &decode_parameters_str);
  if (bytes_read < 0) {
    printf("%s() error, line %d: Can't read from channel\n",
           __func__,
           __LINE__);
    return false;
  }

  if (!output_call_struct.ParseFromString(decode_parameters_str)) {
    printf("%s() error, line %d: Can't parse return buffer\n",
           __func__,
           __LINE__);
    return false;
  }
  if (output_call_struct.function_name() != close_resource_tag) {
    printf("%s() error, line %d: wrong function name tag\n",
           __func__,
           __LINE__);
    return false;
  }
  bool ret = output_call_struct.status();
  if (!ret) {
    return false;
  }
  return true;
}

bool acl_client_dispatch::rpc_add_access_right(
    const string &resource_name,
    const string &delegated_principal,
    const string &right) {

  string   decode_parameters_str;
  string   encode_parameters_str;
  rpc_call input_call_struct;
  rpc_call output_call_struct;
  int      bytes_read = 0;

  // format input buffer, serialize it
  // resource name, right, new_principal
  input_call_struct.set_function_name(add_access_right_tag);
  string *in = input_call_struct.add_str_inputs();
  *in = resource_name;
  string *r = input_call_struct.add_str_inputs();
  *r = right;
  string *new_prin = input_call_struct.add_str_inputs();
  *new_prin = delegated_principal;

  if (!input_call_struct.SerializeToString(&encode_parameters_str)) {
    printf("%s() error, line %d: Can't input\n", __func__, __LINE__);
    return false;
  }

  if (channel_write(channel_descriptor_,
                    encode_parameters_str.size(),
                    (byte *)encode_parameters_str.data())
      < 0) {
    printf("%s() error, line %d: Can't write to channel\n", __func__, __LINE__);
    return false;
  }
#ifdef TEST_SIMULATED_CHANNEL
  g_server.service_request();
#endif
  bytes_read = channel_read(channel_descriptor_, &decode_parameters_str);
  if (bytes_read < 0) {
    printf("%s() error, line %d: Can't read from channel\n",
           __func__,
           __LINE__);
    return false;
  }

  if (!output_call_struct.ParseFromString(decode_parameters_str)) {
    printf("%s() error, line %d: Can't parse return buffer\n",
           __func__,
           __LINE__);
    return false;
  }
  if (output_call_struct.function_name() != add_access_right_tag) {
    printf("%s() error, line %d: wrong function name tag\n",
           __func__,
           __LINE__);
    return false;
  }
  bool ret = output_call_struct.status();
  if (!ret) {
    return false;
  }
  return true;
}


bool acl_client_dispatch::rpc_delete_resource(const string &resource_name,
                                              const string &type) {
  string   decode_parameters_str;
  string   encode_parameters_str;
  rpc_call input_call_struct;
  rpc_call output_call_struct;
  int      bytes_read = 0;

  // format input buffer, serialize it
  input_call_struct.set_function_name(delete_resource_tag);
  string *in = input_call_struct.add_str_inputs();
  *in = resource_name;
  string *resource_type = input_call_struct.add_str_inputs();
  *resource_type = type;

  if (!input_call_struct.SerializeToString(&encode_parameters_str)) {
    printf("%s() error, line %d: Can't input\n", __func__, __LINE__);
    return false;
  }

  if (channel_write(channel_descriptor_,
                    encode_parameters_str.size(),
                    (byte *)encode_parameters_str.data())
      < 0) {
    printf("%s() error, line %d: Can't write to channel\n", __func__, __LINE__);
    return false;
  }
#ifdef TEST_SIMULATED_CHANNEL
  g_server.service_request();
#endif
  bytes_read = channel_read(channel_descriptor_, &decode_parameters_str);
  if (bytes_read < 0) {
    printf("%s() error, line %d: Can't read from channel\n",
           __func__,
           __LINE__);
    return false;
  }

  if (!output_call_struct.ParseFromString(decode_parameters_str)) {
    printf("%s() error, line %d: Can't parse return buffer\n",
           __func__,
           __LINE__);
    return false;
  }
  if (output_call_struct.function_name() != delete_resource_tag) {
    printf("%s() error, line %d: wrong function name tag\n",
           __func__,
           __LINE__);
    return false;
  }
  bool ret = output_call_struct.status();
  if (!ret) {
    return false;
  }
  return true;
}

bool acl_client_dispatch::rpc_create_resource(resource_message &rm) {
  string   decode_parameters_str;
  string   encode_parameters_str;
  rpc_call input_call_struct;
  rpc_call output_call_struct;
  string   serialized_resource;
  int      bytes_read;

  if (!rm.SerializeToString(&serialized_resource)) {
    printf("%s() error, line %d: serialize resource\n", __func__, __LINE__);
    return false;
  }

  // format input buffer, serialize it
  input_call_struct.set_function_name(create_resource_tag);
  string *in = input_call_struct.add_buf_inputs();
  *in = serialized_resource;

  if (!input_call_struct.SerializeToString(&encode_parameters_str)) {
    printf("%s() error, line %d: Can't input\n", __func__, __LINE__);
    return false;
  }

  if (channel_write(channel_descriptor_,
                    encode_parameters_str.size(),
                    (byte *)encode_parameters_str.data())
      < 0) {
    printf("%s() error, line %d: Can't write to channel\n", __func__, __LINE__);
    return false;
  }
#ifdef TEST_SIMULATED_CHANNEL
  g_server.service_request();
#endif
  bytes_read = channel_read(channel_descriptor_, &decode_parameters_str);
  if (bytes_read < 0) {
    printf("%s() error, line %d: Can't read from channel\n",
           __func__,
           __LINE__);
    return false;
  }

  if (!output_call_struct.ParseFromString(decode_parameters_str)) {
    printf("%s() error, line %d: Can't parse return buffer\n",
           __func__,
           __LINE__);
    return false;
  }
  if (output_call_struct.function_name() != create_resource_tag) {
    printf("%s() error, line %d: wrong function name tag\n",
           __func__,
           __LINE__);
    return false;
  }
  bool ret = output_call_struct.status();
  if (!ret) {
    return false;
  }
  return true;
}

bool acl_client_dispatch::rpc_delete_principal(const string &name) {
  printf("%s() error, line %d: delete principal not implemented\n",
         __func__,
         __LINE__);
  return false;
}


bool acl_client_dispatch::rpc_add_principal(const principal_message &pm) {
  string   decode_parameters_str;
  string   encode_parameters_str;
  rpc_call input_call_struct;
  rpc_call output_call_struct;
  string   serialized_principal;
  int      bytes_read;

  if (!pm.SerializeToString(&serialized_principal)) {
    printf("%s() error, line %d: Can't serialize principal\n",
           __func__,
           __LINE__);
    return false;
  }

  // format input buffer, serialize it
  input_call_struct.set_function_name(add_principal_tag);
  string *in = input_call_struct.add_buf_inputs();
  *in = serialized_principal;

  if (!input_call_struct.SerializeToString(&encode_parameters_str)) {
    printf("%s() error, line %d: Can't input\n", __func__, __LINE__);
    return false;
  }

  if (channel_write(channel_descriptor_,
                    encode_parameters_str.size(),
                    (byte *)encode_parameters_str.data())
      < 0) {
    printf("%s() error, line %d: Can't write to channel\n", __func__, __LINE__);
    return false;
  }
#ifdef TEST_SIMULATED_CHANNEL
  g_server.service_request();
#endif
  bytes_read = channel_read(channel_descriptor_, &decode_parameters_str);
  if (bytes_read < 0) {
    printf("%s() error, line %d: Can't read from channel\n",
           __func__,
           __LINE__);
    return false;
  }

  if (!output_call_struct.ParseFromString(decode_parameters_str)) {
    printf("%s() error, line %d: Can't parse return buffer\n",
           __func__,
           __LINE__);
    return false;
  }
  if (output_call_struct.function_name() != add_principal_tag) {
    printf("%s() error, line %d: wrong function name tag\n",
           __func__,
           __LINE__);
    return false;
  }
  bool ret = output_call_struct.status();
  if (!ret) {
    return false;
  }
  return true;
}

acl_server_dispatch::acl_server_dispatch(SSL *channel) {
  channel_descriptor_ = channel;
  initialized_ = true;
}

acl_server_dispatch::~acl_server_dispatch() {}

// returns false if channel is closed or not initialized
bool acl_server_dispatch::service_request() {

  string   decode_parameters_str;
  string   encode_parameters_str;
  rpc_call input_call_struct;
  rpc_call output_call_struct;
  int      bytes_read = 0;

  if (!initialized_) {
    printf("%s() error, line %d: acl_server_dispatch not initialized\n",
           __func__,
           __LINE__);
    return false;
  }

  // read the buffer
  bytes_read = channel_read(channel_descriptor_, &decode_parameters_str);
  if (bytes_read < 0) {
    // check for channel shutdows?
    // int err = SSL_get_error(channel_descriptor_, SSL_ERROR_NONE);
    SSL_shutdown(channel_descriptor_);
    printf("server_dispatch: channel closed\n");
    return false;
  }

  if (!input_call_struct.ParseFromString(decode_parameters_str)) {
    printf("%s() error, line %d: Can't parse call proto %d\n",
           __func__,
           __LINE__,
           (int)decode_parameters_str.size());
    return true;
  }

  if (input_call_struct.function_name() == authenticate_me_tag) {
    if (input_call_struct.str_inputs_size() < 1
        || input_call_struct.buf_inputs_size() < 1) {
      printf("%s() error, line %d: too few input arguments %d\n",
             __func__,
             __LINE__,
             (int)decode_parameters_str.size());
      return true;
    }

    string nonce;
    if (guard_.authenticate_me(input_call_struct.str_inputs(0),
                               input_call_struct.buf_inputs(0),
                               &nonce)) {
      output_call_struct.set_status(true);
      string *nounce_out = output_call_struct.add_buf_outputs();
      nounce_out->assign(nonce.data(), nonce.size());
    } else {
      output_call_struct.set_status(false);
    }
    output_call_struct.set_function_name(authenticate_me_tag);
    if (!output_call_struct.SerializeToString(&encode_parameters_str)) {
      printf("%s() error, line %d: can't encode parameters\n",
             __func__,
             __LINE__);
      return true;  // and the caller never knows
    }

    if (channel_write(channel_descriptor_,
                      encode_parameters_str.size(),
                      (byte *)encode_parameters_str.data())
        < 0) {
      printf("%s() error, line %d: Can't write to channel\n",
             __func__,
             __LINE__);
      return true;
    }

    return true;
  } else if (input_call_struct.function_name() == verify_me_tag) {
    if (input_call_struct.str_inputs_size() < 1) {
      printf("%s() error, line %d: Too few input strings\n",
             __func__,
             __LINE__);
      return true;
    }
    if (input_call_struct.buf_inputs_size() < 1) {
      printf("%s() error, line %d: Too few input buffers\n",
             __func__,
             __LINE__);
      return true;
    }

    if (guard_.verify_me(input_call_struct.str_inputs(0),
                         input_call_struct.buf_inputs(0))) {
      output_call_struct.set_status(true);
    } else {
      output_call_struct.set_status(false);
    }

    output_call_struct.set_function_name(verify_me_tag);
    if (!output_call_struct.SerializeToString(&encode_parameters_str)) {
      printf("%s() error, line %d: can't encode parameters\n",
             __func__,
             __LINE__);
      return true;  // and the caller never knows
    }

    if (channel_write(channel_descriptor_,
                      encode_parameters_str.size(),
                      (byte *)encode_parameters_str.data())
        < 0) {
      printf("%s() error, line %d: Can't write to channel\n",
             __func__,
             __LINE__);
      return true;
    }

    return true;
  } else if (input_call_struct.function_name() == open_resource_tag) {
    if (input_call_struct.str_inputs_size() < 2) {
      printf("%s() error, line %d: too few string inputs\n",
             __func__,
             __LINE__);
      return true;
    }
    output_call_struct.set_function_name(open_resource_tag);
    int desc = -1;
    if (guard_.open_resource(input_call_struct.str_inputs(0),
                             input_call_struct.str_inputs(1),
                             &desc)) {
      output_call_struct.set_status(true);
      output_call_struct.add_int_outputs((google::protobuf::int32)desc);
    } else {
      output_call_struct.set_status(false);
    }
    if (!output_call_struct.SerializeToString(&encode_parameters_str)) {
      printf("%s() error, line %d: can't encode parameters\n",
             __func__,
             __LINE__);
      return true;  // and the caller never knows
    }

    if (channel_write(channel_descriptor_,
                      encode_parameters_str.size(),
                      (byte *)encode_parameters_str.data())
        < 0) {
      printf("%s() error, line %d: Can't write to channel\n",
             __func__,
             __LINE__);
      return true;
    }

    return true;
  } else if (input_call_struct.function_name() == close_resource_tag) {
    if (input_call_struct.str_inputs_size() < 1) {
      printf("%s() error, line %d: Too few string inputs\n",
             __func__,
             __LINE__);
      return true;
    }
    if (input_call_struct.int_inputs_size() < 1) {
      printf("%s() error, line %d: Too few int inputs\n", __func__, __LINE__);
      return true;
    }
    if (guard_.close_resource(input_call_struct.str_inputs(0),
                              input_call_struct.int_inputs(0))) {
      output_call_struct.set_status(true);
    } else {
      output_call_struct.set_status(false);
    }
    output_call_struct.set_function_name(close_resource_tag);
    if (!output_call_struct.SerializeToString(&encode_parameters_str)) {
      printf("%s() error, line %d: can't encode parameters\n",
             __func__,
             __LINE__);
      return true;  // and the caller never knows
    }

    if (channel_write(channel_descriptor_,
                      encode_parameters_str.size(),
                      (byte *)encode_parameters_str.data())
        < 0) {
      printf("%s() error, line %d: Can't write to channel\n",
             __func__,
             __LINE__);
      return true;
    }

    return true;
  } else if (input_call_struct.function_name() == read_resource_tag) {
    if (input_call_struct.str_inputs_size() < 1) {
      printf("%s() error, line %d: too few string resources\n",
             __func__,
             __LINE__);
      return true;
    }
    if (input_call_struct.int_inputs_size() < 2) {
      printf("%s() error, line %d: too few int resources\n",
             __func__,
             __LINE__);
      return true;
    }
    output_call_struct.set_function_name(read_resource_tag);
    string out;
    if (guard_.read_resource(input_call_struct.str_inputs(0),
                             input_call_struct.int_inputs(0),
                             input_call_struct.int_inputs(1),
                             &out)) {
      output_call_struct.set_status(true);
      string *ret_out = output_call_struct.add_buf_outputs();
      ret_out->assign(out.data(), out.size());
    } else {
      output_call_struct.set_status(false);
    }
    if (!output_call_struct.SerializeToString(&decode_parameters_str)) {
      printf("%s() error, line %d: can't encode parameters\n",
             __func__,
             __LINE__);
      return true;  // and the caller never knows
    }

    if (channel_write(channel_descriptor_,
                      decode_parameters_str.size(),
                      (byte *)decode_parameters_str.data())
        < 0) {
      printf("%s() error, line %d: Can't write to channel\n",
             __func__,
             __LINE__);
      return true;
    }

    return true;
  } else if (input_call_struct.function_name() == write_resource_tag) {
    if (input_call_struct.str_inputs_size() < 1) {
      printf("%s() error, line %d: Too few string inputs\n",
             __func__,
             __LINE__);
      return true;
    }
    if (input_call_struct.int_inputs_size() < 2) {
      printf("%s() error, line %d: Too few int inputs\n", __func__, __LINE__);
      return true;
    }
    if (input_call_struct.buf_inputs_size() < 1) {
      printf("%s() error, line %d: Too few buf inputs\n", __func__, __LINE__);
      return true;
    }
    if (guard_.write_resource(input_call_struct.str_inputs(0),
                              input_call_struct.int_inputs(0),
                              input_call_struct.int_inputs(1),
                              (string &)input_call_struct.buf_inputs(0))) {
      output_call_struct.set_status(true);
    } else {
      output_call_struct.set_status(false);
    }
    output_call_struct.set_function_name(write_resource_tag);
    if (!output_call_struct.SerializeToString(&encode_parameters_str)) {
      printf("%s() error, line %d: can't encode parameters\n",
             __func__,
             __LINE__);
      return true;  // and the caller never knows
    }

    if (channel_write(channel_descriptor_,
                      encode_parameters_str.size(),
                      (byte *)encode_parameters_str.data())
        < 0) {
      printf("%s() error, line %d: Can't write to channel\n",
             __func__,
             __LINE__);
      return true;
    }

    return true;
  } else if (input_call_struct.function_name() == add_access_right_tag) {

    // resource_name, right, new_prin
    if (input_call_struct.str_inputs_size() < 3) {
      printf("%s() error, line %d: Too few string inputs\n",
             __func__,
             __LINE__);
      return true;
    }
    if (guard_.add_access_rights(input_call_struct.str_inputs(0),
                                 input_call_struct.str_inputs(1),
                                 input_call_struct.str_inputs(2))) {
      output_call_struct.set_status(true);
    } else {
      output_call_struct.set_status(false);
    }
    output_call_struct.set_function_name(add_access_right_tag);
    if (!output_call_struct.SerializeToString(&encode_parameters_str)) {
      printf("%s() error, line %d: can't encode parameters\n",
             __func__,
             __LINE__);
      return true;  // and the caller never knows
    }

    if (channel_write(channel_descriptor_,
                      encode_parameters_str.size(),
                      (byte *)encode_parameters_str.data())
        < 0) {
      printf("%s() error, line %d: Can't write to channel\n",
             __func__,
             __LINE__);
      return true;
    }
    return true;
  } else if (input_call_struct.function_name() == delete_resource_tag) {
    if (input_call_struct.str_inputs_size() < 2) {
      printf("%s() error, line %d: Too few string inputs\n",
             __func__,
             __LINE__);
      return true;
    }
    if (guard_.delete_resource(input_call_struct.str_inputs(0),
                               input_call_struct.str_inputs(1))) {
      output_call_struct.set_status(true);
    } else {
      output_call_struct.set_status(false);
    }
    output_call_struct.set_function_name(delete_resource_tag);
    if (!output_call_struct.SerializeToString(&encode_parameters_str)) {
      printf("%s() error, line %d: can't encode parameters\n",
             __func__,
             __LINE__);
      return true;  // and the caller never knows
    }

    if (channel_write(channel_descriptor_,
                      encode_parameters_str.size(),
                      (byte *)encode_parameters_str.data())
        < 0) {
      printf("%s() error, line %d: Can't write to channel\n",
             __func__,
             __LINE__);
      return true;
    }
    return true;
  } else if (input_call_struct.function_name() == create_resource_tag) {
    if (input_call_struct.buf_inputs_size() < 1) {
      printf("%s() error, line %d: Too few buf inputs\n", __func__, __LINE__);
      return true;
    }
    resource_message rm;
    bool             ret = true;

    ret = rm.ParseFromString(input_call_struct.buf_inputs(0));
    if (!ret) {
      printf("%s() error, line %d: Can't parse resource message\n",
             __func__,
             __LINE__);
    }
    if (ret && guard_.create_resource(rm)) {
      output_call_struct.set_status(true);
    } else {
      output_call_struct.set_status(false);
    }
    output_call_struct.set_function_name(create_resource_tag);
    if (!output_call_struct.SerializeToString(&encode_parameters_str)) {
      printf("%s() error, line %d: can't encode parameters\n",
             __func__,
             __LINE__);
      return true;  // and the caller never knows
    }

    if (channel_write(channel_descriptor_,
                      encode_parameters_str.size(),
                      (byte *)encode_parameters_str.data())
        < 0) {
      printf("%s() error, line %d: Can't write to channel\n",
             __func__,
             __LINE__);
      return true;
    }
    return true;
  } else if (input_call_struct.function_name() == delete_principal_tag) {
    if (input_call_struct.str_inputs_size() < 1) {
      printf("%s() error, line %d: Too few string inputs\n",
             __func__,
             __LINE__);
      return true;
    }

    output_call_struct.set_function_name(delete_principal_tag);
    if (guard_.delete_principal(input_call_struct.str_inputs(0))) {
      output_call_struct.set_status(true);
    } else {
      output_call_struct.set_status(false);
    }
    if (!output_call_struct.SerializeToString(&encode_parameters_str)) {
      printf("%s() error, line %d: can't encode parameters\n",
             __func__,
             __LINE__);
      return true;  // and the caller never knows
    }

    if (channel_write(channel_descriptor_,
                      encode_parameters_str.size(),
                      (byte *)encode_parameters_str.data())
        < 0) {
      printf("%s() error, line %d: Can't write to channel\n",
             __func__,
             __LINE__);
      return true;
    }
    return true;
  } else if (input_call_struct.function_name() == add_principal_tag) {
    if (input_call_struct.buf_inputs_size() < 1) {
      printf("%s() error, line %d: Too few string inputs\n",
             __func__,
             __LINE__);
      return true;
    }

    output_call_struct.set_function_name(add_principal_tag);
    principal_message pm;
    bool              ret = pm.ParseFromString(input_call_struct.buf_inputs(0));
    if (ret && guard_.add_principal(pm)) {
      output_call_struct.set_status(true);
    } else {
      output_call_struct.set_status(false);
    }
    if (!output_call_struct.SerializeToString(&encode_parameters_str)) {
      printf("%s() error, line %d: can't encode parameters\n",
             __func__,
             __LINE__);
      return true;  // and the caller never knows
    }

    if (channel_write(channel_descriptor_,
                      encode_parameters_str.size(),
                      (byte *)encode_parameters_str.data())
        < 0) {
      printf("%s() error, line %d: Can't write to channel\n",
             __func__,
             __LINE__);
      return true;
    }
    return true;
  } else {
    printf("%s() error, line %d: unknown function %s\n",
           __func__,
           __LINE__,
           input_call_struct.function_name().c_str());
    return true;
  }

  return true;
}


}  // namespace acl_lib
}  // namespace certifier

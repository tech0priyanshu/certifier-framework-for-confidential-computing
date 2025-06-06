// Copyright 2014-2020 John Manferdelli, All Rights Reserved.
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
// File: acl.h

#ifndef _ACL_H__
#define _ACL_H__

#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/ecdsa.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/ec.h>

#include <openssl/asn1.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <mutex>

#include "acl.pb.h"
#include "acl_support.h"

// ---------------------------------------------------------------------------
//  These are acl specific

namespace certifier {
namespace acl_lib {

bool sign_nonce(string &nonce, key_message &k, string *signature);
bool rotate_resource_key(string &resource, key_message &km);

void print_principal_info(const principal_message &pi);
void print_audit_info(const audit_info &inf);
void print_resource_message(const resource_message &rm);
void print_principal_message(const principal_message &pm);
void print_resource_list(const resource_list &rl);
void print_principal_list(const principal_list &pl);

bool add_principal_to_proto_list(const string   &name,
                                 const string   &alg,
                                 const string   &cred,
                                 principal_list *pl);
bool add_resource_to_proto_list(const string  &id,
                                const string  &type,
                                const string  &locat,
                                const string  &t_created,
                                const string  &t_written,
                                resource_list *rl);
bool get_resources_from_file(string &file_name, resource_list *rl);
bool get_principals_from_file(string &file_name, principal_list *pl);
bool save_resources_to_file(resource_list &rl, string &file_name);
bool save_principals_to_file(principal_list &pl, string &file_name);

int find_resource_in_resource_proto_list(const resource_list &rl,
                                         const string        &name);
int find_principal_in_principal_proto_list(const principal_list &pl,
                                           const string         &name);

int on_reader_list(const resource_message &r, const string &name);
int on_writer_list(const resource_message &r, const string &name);
int on_deleter_list(const resource_message &r, const string &name);
int on_owner_list(const resource_message &r, const string &name);

int on_principal_list(const string &name, principal_list &pl);
int on_resource_list(const string &name, resource_list &rl);

bool add_reader_to_resource(const string &name, resource_message *r);
bool add_writer_to_resource(const string &name, resource_message *r);
bool add_deleter_to_resource(const string &name, resource_message *r);
bool add_owner_to_resource(const string &name, resource_message *r);
bool add_principal_to_proto_list(const string   &name,
                                 const string   &alg,
                                 const string   &cred,
                                 principal_list *pl);
bool add_resource_to_proto_list(const string  &id,
                                const string  &locat,
                                const string  &t_created,
                                const string  &t_written,
                                resource_list *rl);

const int max_principal_table_capacity = 250;
class acl_principal_table {
 public:
  acl_principal_table();
  ~acl_principal_table();
  enum { INVALID = 0, VALID = 1 };

  int               capacity_;
  int               num_;
  int               principal_status_[max_principal_table_capacity];
  principal_message principals_[max_principal_table_capacity];
  int               num_managers_;
  string            managers_[max_principal_table_capacity];
  std::mutex        principal_table_mutex_;  // use lock(), unlock()

  bool add_principal_to_table(const string &name,
                              const string &alg,
                              const string &credential);
  bool delete_principal_from_table(const string &name);
  int  find_principal_in_table(const string &name);
  bool load_principal_table_from_list(const principal_list &pl);
  bool save_principal_table_to_list(principal_list *pl);
  bool load_principal_table_from_file(const string &filename);
  bool save_principal_table_to_file(const string &filename);

  void print_entry(int i);
  void print_manager(int i);
};

const int max_resource_table_capacity = 250;
class acl_resource_table {
 public:
  acl_resource_table();
  ~acl_resource_table();
  enum { INVALID = 0, VALID = 1 };

  int              capacity_;
  int              num_;
  int              resource_status_[max_resource_table_capacity];
  resource_message resources_[max_resource_table_capacity];
  std::mutex       resource_table_mutex_;  // use lock(), unlock()

  bool add_resource_to_table(const string &name,
                             const string &type,
                             const string &location);
  bool add_resource_to_table(const resource_message &rm);
  bool delete_resource_from_table(const string &name, const string &type);
  int  find_resource_in_table(const string &name);
  bool load_resource_table_from_list(const resource_list &rl);
  bool save_resource_table_to_list(resource_list *rl);
  bool load_resource_table_from_file(const string &filename);
  bool save_resource_table_to_file(const string &filename);

  void print_entry(int i);
};

class acl_resource_data_element {
 public:
  enum { INVALID = 0, VALID = 1 };
  string resource_name_;
  int    status_;
  int    global_descriptor_;

  acl_resource_data_element();
  ~acl_resource_data_element();
};

const int max_local_descriptors = 50;
class acl_local_descriptor_table {
 public:
  enum { INVALID = 0, VALID = 1 };
  int                       num_;
  int                       capacity_;
  acl_resource_data_element descriptor_entry_[max_local_descriptors];

  acl_local_descriptor_table();
  ~acl_local_descriptor_table();

  int  find_available_descriptor();
  bool free_descriptor(int i, const string &name);
};


extern acl_principal_table g_principal_table;
extern acl_resource_table  g_resource_table;

class channel_guard {
 public:
  channel_guard();
  ~channel_guard();

  bool   initialized_;
  string principal_name_;
  string authentication_algorithm_name_;
  string creds_;
  bool   channel_principal_authenticated_;

  acl_local_descriptor_table descriptor_table_;
  string                     nonce_;
  X509                      *root_cert_;

  void print();

  int find_resource(const string &name);

  bool init_root_cert(const string &asn1_cert_str);
  bool authenticate_me(const string &name,
                       const string &serialized_credentials,
                       string       *nonce);
  bool verify_me(const string &name, const string &signed_nonce);
  bool load_resources(resource_list &rl);

  bool can_read(int resource_entry);
  bool can_write(int resource_entry);
  bool can_delete(int resource_entry);
  bool is_owner(int resource_entry);

  bool access_check(int resource_entry, const string &action);

  // Called from grpc
  bool accept_credentials(const string   &principal_name,
                          const string   &alg,
                          const string   &cred,
                          principal_list *pl);
  bool add_access_rights(const string &resource_name,
                         const string &right,
                         const string &new_prin);
  bool open_resource(const string &resource_name,
                     const string &access_mode,
                     int          *local_descriptor);
  bool read_resource(const string &resource_name,
                     int           local_desciptor,
                     int           n,
                     string       *out);
  bool write_resource(const string &resource_name,
                      int           local_desciptor,
                      int           n,
                      string       &in);
  bool close_resource(const string &resource_name, int local_descriptor);
  bool delete_resource(const string &resource_name, const string &type);
  bool create_resource(resource_message &rm);
  bool add_principal(const principal_message &pm);
  bool delete_principal(const string &name);
};
}  // namespace acl_lib
}  // namespace certifier
#endif

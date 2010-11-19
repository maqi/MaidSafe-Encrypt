/*
* ============================================================================
*
* Copyright [2010] maidsafe.net limited
*
* Created:      2010-04-08
* Company:      maidsafe.net limited
*
* The following source code is property of maidsafe.net limited and is not
* meant for external use.  The use of this code is governed by the license
* file LICENSE.TXT found in the root of this directory and also on
* www.maidsafe.net.
*
* You are not free to copy, amend or otherwise use this source code without
* the explicit written permission of the board of directors of maidsafe.net.
*
* ============================================================================
*/

#ifndef MAIDSAFE_VAULT_BUFFERPACKETSTORE_H_
#define MAIDSAFE_VAULT_BUFFERPACKETSTORE_H_

#include <gtest/gtest_prod.h>
#include <string>
#include <map>

#include "maidsafe/maidsafe.h"
#include "protobuf/sync_data.pb.h"

namespace maidsafe_vault {

class BufferPacketStore {
 public:
  BufferPacketStore() : buffer_packets_(), bp_store_mutex_() {}
  bool StoreBP(const std::string &name, const std::string &ser_bp);
  bool LoadBP(const std::string &name, std::string *ser_bp);
  bool UpdateBP(const std::string &name, const std::string &ser_bp);
  bool DeleteBP(const std::string &name);
  bool HasBP(const std::string &name);
  void ImportMapFromPb(const VaultBufferPacketMap &vault_bp_map);
  VaultBufferPacketMap ExportMapToPb();
  bool InsertBufferPacketFromPb(
      const VaultBufferPacketMap::VaultBufferPacket &vault_bp);
  void Clear();
 private:
  BufferPacketStore &operator=(const BufferPacketStore);
  BufferPacketStore(const BufferPacketStore&);
  FRIEND_TEST(BufferPacketStoreTest, BEH_MAID_BPStoreClean);
  bool DoStoreBP(const std::string &name, const std::string &ser_bp);
  bool DoInsertBufferPacketFromPb(
      const VaultBufferPacketMap::VaultBufferPacket &vault_bp);
  std::map<std::string, std::string> buffer_packets_;
  boost::mutex bp_store_mutex_;
};

}  // namespace maidsafe_vault

#endif  // MAIDSAFE_VAULT_BUFFERPACKETSTORE_H_

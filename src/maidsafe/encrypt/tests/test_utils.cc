/*******************************************************************************
 *  Copyright 2011 maidsafe.net limited                                   *
 *                                                                             *
 *  The following source code is property of maidsafe.net limited and is not   *
 *  meant for external use.  The use of this code is governed by the license   *
 *  file LICENSE.TXT found in the root of this directory and also on           *
 *  www.maidsafe.net.                                                          *
 *                                                                             *
 *  You are not free to copy, amend or otherwise use this source code without  *
 *  the explicit written permission of the board of directors of maidsafe.net. *
 ***************************************************************************//**
 * @file  test_utils.cc
 * @brief Tests for the self-encryption helper functions.
 * @date  2011-04-05
 */

#include <array>
#include <cstdint>
#include <vector>
#include <exception>
#ifdef WIN32
#  pragma warning(push)
#  pragma warning(disable: 4308)
#endif
#include "boost/archive/text_oarchive.hpp"
#ifdef WIN32
#  pragma warning(pop)
#endif
#include "boost/archive/text_iarchive.hpp"
#include "boost/timer.hpp"
#include "maidsafe/common/test.h"
#include "cryptopp/modes.h"
#include "cryptopp/sha.h"
#include "maidsafe/common/crypto.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/encrypt/config.h"
#include "maidsafe/encrypt/data_map.h"
#include "maidsafe/encrypt/utils.h"
#include "maidsafe/encrypt/log.h"
#include "maidsafe/common/memory_chunk_store.h"


namespace maidsafe {

namespace encrypt {


namespace test {

// TEST(SelfEncryptionUtilsTest, BEH_Serialisation) {
//   DataMap data_map;
//   {
//     data_map.content = "abcdefg";
//     data_map.size = 12345;
//     ChunkDetails chunk;
//     chunk.hash = "test123";
//     chunk.size = 10000;
//     data_map.chunks.push_back(chunk);
//     chunk.hash = "test456";
//     chunk.size = 2345;
//     data_map.chunks.push_back(chunk);
//   }
//   std::stringstream ser_data_map;
//   {  // serialise DataMap to string stream
//     boost::archive::text_oarchive oa(ser_data_map);
//     oa << data_map;
//   }
//   {
//     DataMap restored_data_map;
//     boost::archive::text_iarchive ia(ser_data_map);
//     ia >> restored_data_map;
//     EXPECT_EQ(data_map.content, restored_data_map.content);
//     EXPECT_EQ(data_map.size, restored_data_map.size);
//     EXPECT_EQ(data_map.chunks.size(), restored_data_map.chunks.size());
//     EXPECT_EQ(data_map.chunks[0].hash, restored_data_map.chunks[0].hash);
//     EXPECT_EQ(data_map.chunks[0].size, restored_data_map.chunks[0].size);
//     EXPECT_EQ(data_map.chunks[1].hash, restored_data_map.chunks[1].hash);
//     EXPECT_EQ(data_map.chunks[1].size, restored_data_map.chunks[1].size);
//   }
// }

// TEST(SelfEncryptionUtilsTest, XORtest) {
//  // EXPECT_TRUE(XOR("A", "").empty()); // Exception - no pad
//   XORFilter XOR;
//   EXPECT_TRUE(XOR("", "B").empty());
//   EXPECT_EQ(XOR("A", "BB"), XOR("B", "A"));
//   EXPECT_EQ(XOR("AAAA", "BB"), XOR("BBBB", "AA"));
//   const size_t kStringSize(1024*256);
//   std::string str1 = RandomString(kStringSize);
//   std::string str2 = RandomString(kStringSize);
//   std::string obfuscated = XOR(str1, str2);
//   EXPECT_EQ(kStringSize, obfuscated.size());
//   EXPECT_EQ(obfuscated, XOR(str2, str1));
//   EXPECT_EQ(str1, XOR(obfuscated, str2));
//   EXPECT_EQ(str2, XOR(obfuscated, str1));
//   const std::string kZeros(kStringSize, 0);
//   EXPECT_EQ(kZeros, XOR(str1, str1));
//   EXPECT_EQ(str1, XOR(kZeros, str1));
//   const std::string kKnown1("\xa5\x5a");
//   const std::string kKnown2("\x5a\xa5");
//   EXPECT_EQ(std::string("\xff\xff"), XOR(kKnown1, kKnown2));
// 
// }

/*TEST(SelfEncryptionUtilsTest, BEH_SelfEnDecrypt) {
  const std::string data("this is the password");
  std::string enc_hash = Hash(data, kHashingSha512);
  const std::string input(RandomString(1024*256));
  std::string encrypted, decrypted;
  CryptoPP::StringSource(input,
                          true,
       new AESFilter(
             new CryptoPP::StringSink(encrypted),
      enc_hash,
      true));

  CryptoPP::StringSource(encrypted,
                          true,
        new AESFilter(
             new CryptoPP::StringSink(decrypted),
        enc_hash,
        false));
  EXPECT_EQ(decrypted.size(), input.size());
  EXPECT_EQ(decrypted, input);
  EXPECT_NE(encrypted, decrypted);
        }
*/


TEST(SelfEncryptionUtilsTest, BEH_WriteOnly) {
  MemoryChunkStore::HashFunc hash_func = std::bind(&crypto::Hash<crypto::SHA512>,
                                                   std::placeholders::_1);
  std::shared_ptr<MemoryChunkStore> chunk_store(new MemoryChunkStore (true, hash_func));
  SE selfenc(chunk_store);

  std::string content(RandomString(40));

  char *stuff = new char[40];
  std::copy(content.c_str(), content.c_str() + 40, stuff);
  EXPECT_TRUE(selfenc.Write(stuff, 40));
  EXPECT_EQ(0, selfenc.getDataMap().chunks.size());
  EXPECT_EQ(0, selfenc.getDataMap().size);
  EXPECT_EQ(0, selfenc.getDataMap().content_size);
  EXPECT_TRUE(selfenc.FinaliseWrite());
  EXPECT_EQ(40, selfenc.getDataMap().size);
  EXPECT_EQ(40, selfenc.getDataMap().content_size);
  EXPECT_EQ(0, selfenc.getDataMap().chunks.size());
  EXPECT_EQ(static_cast<char>(*stuff),
            static_cast<char>(*selfenc.getDataMap().content));

  EXPECT_TRUE(selfenc.ReInitialise());
  size_t test_data_size(1024*1024*20);
  char *hundredmb = new char[test_data_size];
  for (size_t i = 0; i < test_data_size; ++i) {
    hundredmb[i] = 'a';
  }
  boost::posix_time::ptime time =
        boost::posix_time::microsec_clock::universal_time();
  EXPECT_TRUE(selfenc.Write(hundredmb, test_data_size));
  EXPECT_TRUE(selfenc.FinaliseWrite());
  std::uint64_t duration =
      (boost::posix_time::microsec_clock::universal_time() -
       time).total_microseconds();
  if (duration == 0)
    duration = 1;
  std::cout << "Self-encrypted " << BytesToBinarySiUnits(test_data_size)
             << " in " << (duration / 1000000.0)
             << " seconds at a speed of "
             <<  BytesToBinarySiUnits(test_data_size / (duration / 1000000.0) )
             << "/s" << std::endl;
  DLOG(INFO) << " Created " << selfenc.getDataMap().chunks.size()
            << " Chunks!!" << std::endl;

  char * answer = new char[test_data_size];
  std::shared_ptr<DataMap2> data_map(new DataMap2(data_map_));
  EXPECT_TRUE(selfenc.Read(answer, data_map));

//   for (size_t  i = 0; i < test_data_size; ++i)
//     EXPECT_EQ(answer[i], hundredmb[i]) << i;

  for (int i = 0; i < 64; ++i) {         
    EXPECT_EQ(selfenc.getDataMap().chunks.at(3).pre_hash[i],
              selfenc.getDataMap().chunks.at(4).pre_hash[i]);
    ASSERT_EQ(selfenc.getDataMap().chunks.at(5).hash[i],
              selfenc.getDataMap().chunks.at(3).hash[i]);
   }
}

TEST(SelfEncryptionUtilsTest, BEH_manual_check_write) {
  MemoryChunkStore::HashFunc hash_func =
      std::bind(&crypto::Hash<crypto::SHA512>, std::placeholders::_1);
  std::shared_ptr<MemoryChunkStore> chunk_store(
      new MemoryChunkStore(true, hash_func));
  SE selfenc(chunk_store);

  size_t chunk_size(1024*256); // system default
  size_t num_chunks(10);
  char * extra_content = new char[5]{'1','2','3','4','5'};
  size_t expected_content_size(sizeof(extra_content));
  size_t file_size(chunk_size*num_chunks + expected_content_size); 
  byte *pre_enc_chunk = new byte[chunk_size];
  byte *pad = new byte[144];
  byte *xor_res = new byte[chunk_size];
  byte *prehash = new byte[CryptoPP::SHA512::DIGESTSIZE];
  byte *posthashxor = new byte[CryptoPP::SHA512::DIGESTSIZE];
  byte *postenc = new byte[chunk_size];
  byte *key = new byte[32];
  byte *iv = new byte[16];
  char *pre_enc_file = new char[file_size];

  for (size_t i = 0; i < chunk_size; ++i) {
    pre_enc_chunk[i] = 'b';
  }

  for (size_t i = 0; i < file_size; ++i) {
     pre_enc_file[i] = 'b';
  }

  EXPECT_TRUE(selfenc.ReInitialise());
  EXPECT_TRUE(selfenc.Write(pre_enc_file, file_size));
  EXPECT_TRUE(selfenc.FinaliseWrite());
// Do some testing on results
  EXPECT_EQ(num_chunks,  selfenc.getDataMap().chunks.size());
  EXPECT_EQ(expected_content_size,  selfenc.getDataMap().content_size);
  EXPECT_EQ(file_size, selfenc.getDataMap().size);

  CryptoPP::SHA512().CalculateDigest(prehash, pre_enc_chunk, chunk_size);

  for (int i = 0; i < 64; ++i) {
    pad[i] = prehash[i];
    pad[i+64] = prehash[i];
  }
  for (int i = 0; i < 16; ++i) {
    pad[i+128] = prehash[i+48];
  }
  std::copy(prehash, prehash + 32, key);
  std::copy(prehash + 32, prehash + 48, iv);

  CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption enc(key, 32, iv);
  enc.ProcessData(postenc, pre_enc_chunk, chunk_size);


  for (size_t i = 0; i < chunk_size; ++i) {
    xor_res[i] = postenc[i]^pad[i%144];
  }

  CryptoPP::SHA512().CalculateDigest(posthashxor, xor_res, chunk_size);

  for (int i = 0; i < 64; ++i) {
    ASSERT_EQ(prehash[i], selfenc.getDataMap().chunks[4].pre_hash[i]);
    ASSERT_EQ(posthashxor[i], selfenc.getDataMap().chunks[4].hash[i]);
  }

  // check chunks' hashes - should be equal for repeated single character input
  bool match(true);
  for (size_t i = 0; i < selfenc.getDataMap().chunks.size(); ++i) {
    for (size_t j = i; j < selfenc.getDataMap().chunks.size(); ++j) {
      for (int k = 0; k < CryptoPP::SHA512::DIGESTSIZE ; ++k) {
        if (selfenc.getDataMap().chunks[i].hash[k] !=
                selfenc.getDataMap().chunks[j].hash[k])
          match = false;
      }
      EXPECT_TRUE(match);
      match = true;
    }
  }


}

}  // namespace test

}  // namespace encrypt

}  // namespace maidsafe

/*
* ============================================================================
*
* Copyright [2009] maidsafe.net limited
*
* Description:  Test for VaultServiceLogic class using mock RPCs
* Version:      1.0
* Created:      2010-01-08-12.33.18
* Revision:     none
* Compiler:     gcc
* Author:       Fraser Hutchison (fh), fraser.hutchison@maidsafe.net
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

#include <gmock/gmock.h>
#include <maidsafe/kademlia_service_messages.pb.h>

#include "maidsafe/client/packetfactory.h"
#include "maidsafe/vault/vaultrpc.h"
#include "maidsafe/vault/vaultservicelogic.h"

namespace test_vsl {

void CopyResult(const int &response,
                boost::mutex *mutex,
                boost::condition_variable *cv,
                int *result) {
  boost::mutex::scoped_lock lock(*mutex);
  *result = response;
  cv->notify_one();
}

enum FindNodesResponseType {
  kFailParse,
  kResultFail,
  kTooFewContacts,
  kGood
};

std::string MakeFindNodesResponse(const FindNodesResponseType &type) {
  if (type == kFailParse)
    return "It's not going to parse.";
  crypto::Crypto co;
  co.set_hash_algorithm(crypto::SHA_512);
  std::string ser_node;
  kad::FindResponse find_response;
  if (type == kResultFail)
    find_response.set_result(kad::kRpcResultFailure);
  else
    find_response.set_result(kad::kRpcResultSuccess);
  int contact_count(kad::K);
  if (type == kTooFewContacts)
    contact_count = 1;
  // Set all IDs close to value of account we're going to be looking for to
  // avoid test node replacing one of these after the kad FindKNodes
  std::string account_owner(co.Hash("Account Owner", "", crypto::STRING_STRING,
      false));
  std::string account_name(co.Hash(account_owner + kAccount, "",
      crypto::STRING_STRING, false));
  char x = 'a';
  for (int i = 0; i < contact_count; ++i, ++x) {
    kad::Contact node(account_name.replace(account_name.size() - 1, 1, 1, x),
        "192.168.1.1", 5000 + i);
    node.SerialiseToString(&ser_node);
    find_response.add_closest_nodes(ser_node);
  }
  find_response.SerializeToString(&ser_node);
  return ser_node;
}

void RunCallback(const std::string &find_nodes_response,
                 const base::callback_func_type &callback) {
  callback(find_nodes_response);
}

void DoneRun(const int &min_delay,
             const int &max_delay,
             google::protobuf::Closure* callback) {
  int min(min_delay);
  if (min < 0)
    min = 0;
  int diff = max_delay - min;
  if (diff < 1)
    diff = 1;
  int sleep_time(base::random_32bit_uinteger() % diff + min);
  boost::this_thread::sleep(boost::posix_time::milliseconds(sleep_time));
  callback->Run();
}

void ThreadedDoneRun(const int &min_delay,
                     const int &max_delay,
                     google::protobuf::Closure* callback) {
  boost::thread(DoneRun, min_delay, max_delay, callback);
}

std::string MaxDist(const std::string &hex_input) {
  std::string output;
  for (size_t i = 0; i < hex_input.size(); ++i) {
    char x;
    switch (hex_input.at(i)) {
      case '0':
        x = 'f';
        break;
      case '1':
        x = 'e';
        break;
      case '2':
        x = 'd';
        break;
      case '3':
        x = 'c';
        break;
      case '4':
        x = 'b';
        break;
      case '5':
        x = 'a';
        break;
      case '6':
        x = '9';
        break;
      case '7':
        x = '8';
        break;
      case '8':
        x = '7';
        break;
      case '9':
        x = '6';
        break;
      case 'a':
        x = '5';
        break;
      case 'b':
        x = '4';
        break;
      case 'c':
        x = '3';
        break;
      case 'd':
        x = '2';
        break;
      case 'e':
        x = '1';
        break;
      case 'f':
        x = '0';
        break;
    }
    output += x;
  }
  return output;
}

}  // namespace test_vsl

namespace maidsafe_vault {

class MockVaultRpcs : public VaultRpcs {
 public:
  MockVaultRpcs(transport::Transport *transport,
                rpcprotocol::ChannelManager *channel_manager)
                     : VaultRpcs(transport, channel_manager) {}
  MOCK_METHOD6(AddToReferenceList, void(
      const kad::Contact &peer,
      bool local,
      maidsafe::AddToReferenceListRequest *add_to_reference_list_request,
      maidsafe::AddToReferenceListResponse *add_to_reference_list_response,
      rpcprotocol::Controller *controller,
      google::protobuf::Closure *done));
  MOCK_METHOD6(AmendAccount, void(
      const kad::Contact &peer,
      bool local,
      maidsafe::AmendAccountRequest *amend_account_request,
      maidsafe::AmendAccountResponse *amend_account_response,
      rpcprotocol::Controller *controller,
      google::protobuf::Closure *done));
  MOCK_METHOD6(AccountStatus, void(
      const kad::Contact &peer,
      bool local,
      maidsafe::AccountStatusRequest *get_account_status_request,
      maidsafe::AccountStatusResponse *get_account_status_response,
      rpcprotocol::Controller *controller,
      google::protobuf::Closure *done));
};

class VaultServiceLogicTest : public testing::Test {
 protected:
  VaultServiceLogicTest()
      : pmid_(),
        hex_pmid_(),
        pmid_private_(),
        pmid_public_(),
        pmid_public_signature_(),
        mock_rpcs_(NULL, NULL),
        fail_parse_result_(
            test_vsl::MakeFindNodesResponse(test_vsl::kFailParse)),
        fail_result_(test_vsl::MakeFindNodesResponse(test_vsl::kResultFail)),
        few_result_(test_vsl::MakeFindNodesResponse(test_vsl::kTooFewContacts)),
        good_result_(test_vsl::MakeFindNodesResponse(test_vsl::kGood)),
        good_result_less_one_(),
        our_contact_(),
        good_contacts_() {}
  virtual ~VaultServiceLogicTest() {}
  virtual void SetUp() {
    crypto_.set_hash_algorithm(crypto::SHA_512);
    crypto_.set_symm_algorithm(crypto::AES_256);
    pmid_keys_.GenerateKeys(maidsafe::kRsaKeySize);
    pmid_private_ = pmid_keys_.private_key();
    pmid_public_ = pmid_keys_.public_key();
    // PMID isn't signed by its own private key in production code, but this
    // is quicker rather than generating a new set of keys
    pmid_public_signature_ = crypto_.AsymSign(pmid_public_, "", pmid_private_,
        crypto::STRING_STRING);
    hex_pmid_ = crypto_.Hash(pmid_public_ + pmid_public_signature_, "",
        crypto::STRING_STRING, true);
    pmid_ = base::DecodeFromHex(hex_pmid_);
    our_contact_ = kad::Contact(pmid_, "192.168.10.10", 8008);
    std::string ser_our_contact;
    our_contact_.SerialiseToString(&ser_our_contact);
    kad::FindResponse find_response, good_find_response_less_one;
    kad::Contact contact;
    std::string ser_contact;
    ASSERT_TRUE(find_response.ParseFromString(good_result_));
    good_find_response_less_one.set_result(kad::kRpcResultSuccess);
    for (int i = 0; i < find_response.closest_nodes_size(); ++i) {
      ser_contact = find_response.closest_nodes(i);
      ASSERT_TRUE(contact.ParseFromString(ser_contact));
      good_contacts_.push_back(contact);
      if (i < find_response.closest_nodes_size() - 1) {
        good_find_response_less_one.add_closest_nodes(ser_contact);
      }
    }
    good_find_response_less_one.SerializeToString(&good_result_less_one_);
  }

  crypto::RsaKeyPair pmid_keys_;
  std::string pmid_, hex_pmid_, pmid_private_, pmid_public_;
  std::string pmid_public_signature_;
  crypto::Crypto crypto_;
  MockVaultRpcs mock_rpcs_;
  std::string fail_parse_result_, fail_result_, few_result_, good_result_;
  std::string good_result_less_one_;
  kad::Contact our_contact_;
  std::vector<kad::Contact> good_contacts_;

 private:
  VaultServiceLogicTest(const VaultServiceLogicTest&);
  VaultServiceLogicTest &operator=(const VaultServiceLogicTest&);
};

TEST_F(VaultServiceLogicTest, BEH_MAID_VSL_Offline) {
  VaultServiceLogic vsl(&mock_rpcs_, NULL);

  maidsafe::StoreContract sc;
  ASSERT_EQ(kVaultOffline, vsl.AddToRemoteRefList("x", sc));

  maidsafe::AmendAccountRequest aar;
  boost::mutex mutex;
  boost::condition_variable cv;
  int result(kGeneralError);
  Callback cb = boost::bind(&test_vsl::CopyResult, _1, &mutex, &cv, &result);
  vsl.AmendRemoteAccount(aar, cb);
  {
    boost::mutex::scoped_lock lock(mutex);
    while (result == kGeneralError) {
      cv.wait(lock);
    }
  }
  ASSERT_EQ(kVaultOffline, result);

  maidsafe::AccountStatusRequest asr;
  ASSERT_EQ(kVaultOffline, vsl.RemoteVaultAbleToStore(asr));
}

class MockVsl : public VaultServiceLogic {
 public:
  MockVsl(VaultRpcs *vault_rpcs, kad::KNode *knode)
      : VaultServiceLogic(vault_rpcs, knode) {}
  MOCK_METHOD2(FindCloseNodes, void(const std::string &kad_key,
                                    const base::callback_func_type &callback));
  MOCK_METHOD1(AddressIsLocal, bool(const kad::Contact &peer));
};

TEST_F(VaultServiceLogicTest, BEH_MAID_VSL_FindKNodes) {
  // Setup
  MockVsl vsl(&mock_rpcs_, NULL);
  vsl.online_ = true;
  std::vector<kad::Contact> contacts;
  kad::Contact dummy_contact = kad::Contact(crypto_.Hash("Dummy", "",
      crypto::STRING_STRING, false), "192.168.1.0", 4999);

  // Expectations
  EXPECT_CALL(vsl, FindCloseNodes("x", testing::_))
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, fail_parse_result_, _1))))  // 2
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, fail_result_, _1))))  // Call 3
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, few_result_, _1))))  // Call 4
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, good_result_, _1))))  // Call 5
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, good_result_, _1))));  // Call 6

  // Call 1
  ASSERT_EQ(kVaultServiceError, vsl.FindKNodes("x", NULL));

  // Call 2
  contacts.push_back(dummy_contact);
  ASSERT_EQ(size_t(1), contacts.size());
  ASSERT_EQ(kVaultServiceFindNodesError, vsl.FindKNodes("x", &contacts));
  ASSERT_EQ(size_t(0), contacts.size());

  // Call 3
  contacts.push_back(dummy_contact);
  ASSERT_EQ(size_t(1), contacts.size());
  ASSERT_EQ(kVaultServiceFindNodesFailure, vsl.FindKNodes("x", &contacts));
  ASSERT_EQ(size_t(0), contacts.size());

  // Call 4
  ASSERT_EQ(kSuccess, vsl.FindKNodes("x", &contacts));
  ASSERT_EQ(size_t(2), contacts.size());

  // Call 5
  ASSERT_EQ(kSuccess, vsl.FindKNodes("x", &contacts));
  ASSERT_EQ(size_t(16), contacts.size());

  // Call 6
  contacts.push_back(dummy_contact);
  ASSERT_EQ(kSuccess, vsl.FindKNodes("x", &contacts));
  ASSERT_EQ(size_t(16), contacts.size());
}

TEST_F(VaultServiceLogicTest, FUNC_MAID_VSL_AddToRemoteRefList) {
  // Setup
  MockVsl vsl(&mock_rpcs_, NULL);
  vsl.non_hex_pmid_ = pmid_;
  vsl.pmid_public_signature_ = pmid_public_signature_;
  vsl.pmid_private_ = pmid_private_;
  vsl.online_ = true;
  vsl.our_details_ = our_contact_;
  maidsafe::StoreContract store_contract;

  std::vector<maidsafe::AddToReferenceListResponse> good_responses;
  std::vector<maidsafe::AddToReferenceListResponse> good_responses_less_one;
  for (size_t i = 0; i < good_contacts_.size(); ++i) {
    maidsafe::AddToReferenceListResponse add_ref_response;
    add_ref_response.set_result(kAck);
    add_ref_response.set_pmid(good_contacts_.at(i).node_id());
    good_responses.push_back(add_ref_response);
    if (i < good_contacts_.size() - 1)
      good_responses_less_one.push_back(add_ref_response);
  }
  std::vector<maidsafe::AddToReferenceListResponse>
      bad_pmid_responses(good_responses);
  std::vector<maidsafe::AddToReferenceListResponse>
      too_few_ack_responses(good_responses);
  std::vector<maidsafe::AddToReferenceListResponse>
      fail_initialise_responses(good_responses);
  for (size_t i = vsl.kKadStoreThreshold_ - 1; i < good_contacts_.size(); ++i) {
    bad_pmid_responses.at(i).set_pmid(good_contacts_.at(i - 1).node_id());
    too_few_ack_responses.at(i).set_result(kNack);
    fail_initialise_responses.at(i).clear_result();
  }
  // Set chunkname as far as possible from our ID so we don't get added to
  // vector of close nodes in vsl.HandleFindKNodesResponse
  std::string far_chunkname = base::DecodeFromHex(test_vsl::MaxDist(hex_pmid_));

  // Expectations
  EXPECT_CALL(vsl, FindCloseNodes(far_chunkname, testing::_))
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, fail_parse_result_, _1))))  // 1
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, fail_result_, _1))))  // Call 2
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, few_result_, _1))))  // Call 3
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, good_result_, _1))))  // Call 4
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, good_result_, _1))))  // Call 6
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, good_result_, _1))))  // Call 7
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, good_result_, _1))));  // Call 8
  EXPECT_CALL(vsl, FindCloseNodes(pmid_, testing::_))  // Call 5
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, good_result_less_one_, _1))));

  for (size_t i = 0; i < good_contacts_.size(); ++i) {
    if (i < good_contacts_.size() - 1) {
      EXPECT_CALL(vsl, AddressIsLocal(good_contacts_.at(i)))
          .WillOnce(testing::Return(true))  // Call 4
          .WillOnce(testing::Return(false))  // Call 5
          .WillOnce(testing::Return(true))  // Call 6
          .WillOnce(testing::Return(true))  // Call 7
          .WillOnce(testing::Return(true));  // Call 8
    } else {
      EXPECT_CALL(vsl, AddressIsLocal(good_contacts_.at(i)))
          .WillOnce(testing::Return(true))  // Call 4
          .WillOnce(testing::Return(true))  // Call 6
          .WillOnce(testing::Return(true))  // Call 7
          .WillOnce(testing::Return(true));  // Call 8
    }
  }

  for (size_t i = 0; i < good_contacts_.size(); ++i) {
      EXPECT_CALL(mock_rpcs_, AddToReferenceList(good_contacts_.at(i), true,
          testing::_, testing::_, testing::_, testing::_))
              .WillOnce(DoAll(testing::SetArgumentPointee<3>(
                                    good_responses.at(i)),  // Call 4
                              testing::WithArgs<5>(testing::Invoke(
                  boost::bind(&test_vsl::ThreadedDoneRun, 100, 5000, _1)))))
              .WillOnce(DoAll(testing::SetArgumentPointee<3>(
                                    bad_pmid_responses.at(i)),  // Call 6
                              testing::WithArgs<5>(testing::Invoke(
                  boost::bind(&test_vsl::ThreadedDoneRun, 100, 5000, _1)))))
              .WillOnce(DoAll(testing::SetArgumentPointee<3>(
                                    too_few_ack_responses.at(i)),  // Call 7
                              testing::WithArgs<5>(testing::Invoke(
                  boost::bind(&test_vsl::ThreadedDoneRun, 100, 5000, _1)))))
              .WillOnce(DoAll(testing::SetArgumentPointee<3>(
                                    fail_initialise_responses.at(i)),  // Call 8
                              testing::WithArgs<5>(testing::Invoke(
                  boost::bind(&test_vsl::ThreadedDoneRun, 100, 5000, _1)))));
  }
  for (size_t i = 0; i < good_contacts_.size() - 1; ++i) {
      EXPECT_CALL(mock_rpcs_, AddToReferenceList(good_contacts_.at(i), false,
          testing::_, testing::_, testing::_, testing::_))  // Call 5
              .WillOnce(DoAll(testing::SetArgumentPointee<3>(
                                    good_responses_less_one.at(i)),
                              testing::WithArgs<5>(testing::Invoke(
                  boost::bind(&test_vsl::ThreadedDoneRun, 100, 5000, _1)))));
  }

  // Call 1 - FindKNodes fails (NULL pointer)
  ASSERT_EQ(kVaultServiceFindNodesError,
            vsl.AddToRemoteRefList(far_chunkname, store_contract));

  // Call 2 - FindKNodes returns kNack
  ASSERT_EQ(kVaultServiceFindNodesFailure,
            vsl.AddToRemoteRefList(far_chunkname, store_contract));

  // Call 3 - FindKnodes only returns 1 node
  ASSERT_EQ(kVaultServiceFindNodesTooFew,
            vsl.AddToRemoteRefList(far_chunkname, store_contract));

  // Call 4 - All OK
  ASSERT_EQ(kSuccess, vsl.AddToRemoteRefList(far_chunkname, store_contract));

  // Call 5 - All OK - we're close to chunkname, so we replace contact 16
  ASSERT_EQ(kSuccess, vsl.AddToRemoteRefList(pmid_, store_contract));

  // Call 6 - Five responses have incorrect PMID
  ASSERT_EQ(kAddToRefResponseError,
            vsl.AddToRemoteRefList(far_chunkname, store_contract));

  // Call 7 - Five responses return kNack
  ASSERT_EQ(kAddToRefResponseFailed,
            vsl.AddToRemoteRefList(far_chunkname, store_contract));

  // Call 8 - Five responses don't have result set
  ASSERT_EQ(kAddToRefResponseUninitialised,
            vsl.AddToRemoteRefList(far_chunkname, store_contract));
}

TEST_F(VaultServiceLogicTest, FUNC_MAID_VSL_AmendRemoteAccount) {
  // Setup
  MockVsl vsl(&mock_rpcs_, NULL);
  vsl.non_hex_pmid_ = pmid_;
  vsl.pmid_public_signature_ = pmid_public_signature_;
  vsl.pmid_private_ = pmid_private_;
  vsl.online_ = true;
  vsl.our_details_ = our_contact_;

  std::vector<maidsafe::AmendAccountResponse> good_responses;
  std::vector<maidsafe::AmendAccountResponse> good_responses_less_one;
  for (size_t i = 0; i < good_contacts_.size(); ++i) {
    maidsafe::AmendAccountResponse amend_acc_response;
    amend_acc_response.set_result(kAck);
    amend_acc_response.set_pmid(good_contacts_.at(i).node_id());
    good_responses.push_back(amend_acc_response);
    if (i < good_contacts_.size() - 1)
      good_responses_less_one.push_back(amend_acc_response);
  }
  std::vector<maidsafe::AmendAccountResponse>
      bad_pmid_responses(good_responses);
  std::vector<maidsafe::AmendAccountResponse>
      too_few_ack_responses(good_responses);
  std::vector<maidsafe::AmendAccountResponse>
      fail_initialise_responses(good_responses);
  for (size_t i = vsl.kKadStoreThreshold_ - 1; i < good_contacts_.size(); ++i) {
    bad_pmid_responses.at(i).set_pmid(good_contacts_.at(i - 1).node_id());
    too_few_ack_responses.at(i).set_result(kNack);
    fail_initialise_responses.at(i).clear_result();
  }

  std::string account_owner(crypto_.Hash("Account Owner", "",
      crypto::STRING_STRING, false));
  std::string account_name(crypto_.Hash(account_owner + kAccount, "",
      crypto::STRING_STRING, false));
  maidsafe::AmendAccountRequest request;
  request.set_amendment_type(maidsafe::AmendAccountRequest::kSpaceGivenInc);
  request.set_account_pmid(account_owner);
  maidsafe::SignedSize *mutable_signed_size = request.mutable_signed_size();
  mutable_signed_size->set_data_size(10);
  mutable_signed_size->set_pmid(pmid_);
  mutable_signed_size->set_signature(crypto_.AsymSign(base::itos_ull(10), "",
                                     pmid_private_, crypto::STRING_STRING));
  mutable_signed_size->set_public_key(pmid_public_);
  mutable_signed_size->set_public_key_signature(pmid_public_signature_);
  request.set_chunkname(crypto_.Hash("Chunkname", "", crypto::STRING_STRING,
      false));

  boost::mutex mutex;
  boost::condition_variable cv;
  int result(kGeneralError);
  Callback cb = boost::bind(&test_vsl::CopyResult, _1, &mutex, &cv, &result);

  // Expectations
  EXPECT_CALL(vsl, FindCloseNodes(account_name, testing::_))
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, fail_parse_result_, _1))))  // 1
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, fail_result_, _1))))  // Call 2
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, few_result_, _1))))  // Call 3
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, good_result_, _1))))  // Call 4
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, good_result_less_one_, _1))))
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, good_result_, _1))))  // Call 6
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, good_result_, _1))))  // Call 7
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, good_result_, _1))));  // Call 8

  for (size_t i = 0; i < good_contacts_.size(); ++i) {
    if (i < good_contacts_.size() - 1) {
      EXPECT_CALL(vsl, AddressIsLocal(good_contacts_.at(i)))
          .WillOnce(testing::Return(true))  // Call 4
          .WillOnce(testing::Return(false))  // Call 5
          .WillOnce(testing::Return(true))  // Call 6
          .WillOnce(testing::Return(true))  // Call 7
          .WillOnce(testing::Return(true));  // Call 8
    } else {
      EXPECT_CALL(vsl, AddressIsLocal(good_contacts_.at(i)))
          .WillOnce(testing::Return(true))  // Call 4
          .WillOnce(testing::Return(true))  // Call 6
          .WillOnce(testing::Return(true))  // Call 7
          .WillOnce(testing::Return(true));  // Call 8
    }
  }

  for (size_t i = 0; i < good_contacts_.size(); ++i) {
      EXPECT_CALL(mock_rpcs_, AmendAccount(good_contacts_.at(i), true,
          testing::_, testing::_, testing::_, testing::_))
              .WillOnce(DoAll(testing::SetArgumentPointee<3>(
                                    good_responses.at(i)),  // Call 4
                              testing::WithArgs<5>(testing::Invoke(
                  boost::bind(&test_vsl::ThreadedDoneRun, 100, 5000, _1)))))
              .WillOnce(DoAll(testing::SetArgumentPointee<3>(
                                    bad_pmid_responses.at(i)),  // Call 6
                              testing::WithArgs<5>(testing::Invoke(
                  boost::bind(&test_vsl::ThreadedDoneRun, 100, 5000, _1)))))
              .WillOnce(DoAll(testing::SetArgumentPointee<3>(
                                    too_few_ack_responses.at(i)),  // Call 7
                              testing::WithArgs<5>(testing::Invoke(
                  boost::bind(&test_vsl::ThreadedDoneRun, 100, 5000, _1)))))
              .WillOnce(DoAll(testing::SetArgumentPointee<3>(
                                    fail_initialise_responses.at(i)),  // Call 8
                              testing::WithArgs<5>(testing::Invoke(
                  boost::bind(&test_vsl::ThreadedDoneRun, 100, 5000, _1)))));
  }
  for (size_t i = 0; i < good_contacts_.size() - 1; ++i) {
      EXPECT_CALL(mock_rpcs_, AmendAccount(good_contacts_.at(i), false,
          testing::_, testing::_, testing::_, testing::_))  // Call 5
              .WillOnce(DoAll(testing::SetArgumentPointee<3>(
                                    good_responses_less_one.at(i)),
                              testing::WithArgs<5>(testing::Invoke(
                  boost::bind(&test_vsl::ThreadedDoneRun, 100, 5000, _1)))));
  }

  // Call 1 - FindKNodes fails (NULL pointer)
  vsl.AmendRemoteAccount(request, cb);
  {
    boost::mutex::scoped_lock lock(mutex);
    while (result == kGeneralError) {
      cv.wait(lock);
    }
  }
  ASSERT_EQ(kVaultServiceFindNodesError, result);

  // Call 2 - FindKNodes returns kNack
  result = kGeneralError;
  vsl.AmendRemoteAccount(request, cb);
  {
    boost::mutex::scoped_lock lock(mutex);
    while (result == kGeneralError) {
      cv.wait(lock);
    }
  }
  ASSERT_EQ(kVaultServiceFindNodesFailure, result);

  // Call 3 - FindKnodes only returns 1 node
  result = kGeneralError;
  vsl.AmendRemoteAccount(request, cb);
  {
    boost::mutex::scoped_lock lock(mutex);
    while (result == kGeneralError) {
      cv.wait(lock);
    }
  }
  ASSERT_EQ(kVaultServiceFindNodesTooFew, result);

  // Call 4 - All OK
  result = kGeneralError;
  vsl.AmendRemoteAccount(request, cb);
  {
    boost::mutex::scoped_lock lock(mutex);
    while (result == kGeneralError) {
      cv.wait(lock);
    }
  }
  ASSERT_EQ(kSuccess, result);

  // Call 5 - All OK - we're close to chunkname, so we replace contact 16
  result = kGeneralError;
  vsl.AmendRemoteAccount(request, cb);
  {
    boost::mutex::scoped_lock lock(mutex);
    while (result == kGeneralError) {
      cv.wait(lock);
    }
  }
  ASSERT_EQ(kSuccess, result);

  // Call 6 - Five responses have incorrect PMID
  result = kGeneralError;
  vsl.AmendRemoteAccount(request, cb);
  {
    boost::mutex::scoped_lock lock(mutex);
    while (result == kGeneralError) {
      cv.wait(lock);
    }
  }
  ASSERT_EQ(kAmendAccountResponseError, result);

  // Call 7 - Five responses return kNack
  result = kGeneralError;
  vsl.AmendRemoteAccount(request, cb);
  {
    boost::mutex::scoped_lock lock(mutex);
    while (result == kGeneralError) {
      cv.wait(lock);
    }
  }
  ASSERT_EQ(kAmendAccountResponseFailed, result);

  // Call 8 - Five responses don't have result set
  result = kGeneralError;
  vsl.AmendRemoteAccount(request, cb);
  {
    boost::mutex::scoped_lock lock(mutex);
    while (result == kGeneralError) {
      cv.wait(lock);
    }
  }
  ASSERT_EQ(kAmendAccountResponseUninitialised, result);
}

TEST_F(VaultServiceLogicTest, FUNC_MAID_VSL_RemoteVaultAbleToStore) {
  // Setup
  MockVsl vsl(&mock_rpcs_, NULL);
  vsl.non_hex_pmid_ = pmid_;
  vsl.pmid_public_signature_ = pmid_public_signature_;
  vsl.pmid_private_ = pmid_private_;
  vsl.online_ = true;
  vsl.our_details_ = our_contact_;
  maidsafe::StoreContract store_contract;

  std::vector<maidsafe::AccountStatusResponse> good_responses;
  std::vector<maidsafe::AccountStatusResponse> good_responses_less_one;
  for (size_t i = 0; i < good_contacts_.size(); ++i) {
    maidsafe::AccountStatusResponse acc_status_response;
    acc_status_response.set_result(kAck);
    acc_status_response.set_pmid(good_contacts_.at(i).node_id());
    good_responses.push_back(acc_status_response);
    if (i < good_contacts_.size() - 1)
      good_responses_less_one.push_back(acc_status_response);
  }
  std::vector<maidsafe::AccountStatusResponse>
      bad_pmid_responses(good_responses);
  std::vector<maidsafe::AccountStatusResponse>
      too_few_ack_responses(good_responses);
  std::vector<maidsafe::AccountStatusResponse>
      fail_initialise_responses(good_responses);
  for (size_t i = kKadTrustThreshold - 1; i < good_contacts_.size(); ++i) {
    bad_pmid_responses.at(i).set_pmid(good_contacts_.at(i - 1).node_id());
    too_few_ack_responses.at(i).set_result(kNack);
    fail_initialise_responses.at(i).clear_result();
  }
  std::string account_owner(crypto_.Hash("Account Owner", "",
      crypto::STRING_STRING, false));
  std::string account_name(crypto_.Hash(account_owner + kAccount, "",
      crypto::STRING_STRING, false));
  maidsafe::AccountStatusRequest request;
  request.set_account_pmid(account_owner);

  // Expectations
  EXPECT_CALL(vsl, FindCloseNodes(account_name, testing::_))
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, fail_parse_result_, _1))))  // 1
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, fail_result_, _1))))  // Call 2
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, few_result_, _1))))  // Call 3
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, good_result_, _1))))  // Call 4
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, good_result_less_one_, _1))))
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, good_result_, _1))))  // Call 6
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, good_result_, _1))))  // Call 7
      .WillOnce(testing::WithArgs<1>(testing::Invoke(
          boost::bind(&test_vsl::RunCallback, good_result_, _1))));  // Call 8

  for (size_t i = 0; i < good_contacts_.size(); ++i) {
    if (i < good_contacts_.size() - 1) {
      EXPECT_CALL(vsl, AddressIsLocal(good_contacts_.at(i)))
          .WillOnce(testing::Return(true))  // Call 4
          .WillOnce(testing::Return(false))  // Call 5
          .WillOnce(testing::Return(true))  // Call 6
          .WillOnce(testing::Return(true))  // Call 7
          .WillOnce(testing::Return(true));  // Call 8
    } else {
      EXPECT_CALL(vsl, AddressIsLocal(good_contacts_.at(i)))
          .WillOnce(testing::Return(true))  // Call 4
          .WillOnce(testing::Return(true))  // Call 6
          .WillOnce(testing::Return(true))  // Call 7
          .WillOnce(testing::Return(true));  // Call 8
    }
  }

  for (size_t i = 0; i < good_contacts_.size(); ++i) {
      EXPECT_CALL(mock_rpcs_, AccountStatus(good_contacts_.at(i), true,
          testing::_, testing::_, testing::_, testing::_))
              .WillOnce(DoAll(testing::SetArgumentPointee<3>(
                                    good_responses.at(i)),  // Call 4
                              testing::WithArgs<5>(testing::Invoke(
                  boost::bind(&test_vsl::ThreadedDoneRun, 100, 5000, _1)))))
              .WillOnce(DoAll(testing::SetArgumentPointee<3>(
                                    bad_pmid_responses.at(i)),  // Call 6
                              testing::WithArgs<5>(testing::Invoke(
                  boost::bind(&test_vsl::ThreadedDoneRun, 100, 5000, _1)))))
              .WillOnce(DoAll(testing::SetArgumentPointee<3>(
                                    too_few_ack_responses.at(i)),  // Call 7
                              testing::WithArgs<5>(testing::Invoke(
                  boost::bind(&test_vsl::ThreadedDoneRun, 100, 5000, _1)))))
              .WillOnce(DoAll(testing::SetArgumentPointee<3>(
                                    fail_initialise_responses.at(i)),  // Call 8
                              testing::WithArgs<5>(testing::Invoke(
                  boost::bind(&test_vsl::ThreadedDoneRun, 100, 5000, _1)))));
  }
  for (size_t i = 0; i < good_contacts_.size() - 1; ++i) {
      EXPECT_CALL(mock_rpcs_, AccountStatus(good_contacts_.at(i), false,
          testing::_, testing::_, testing::_, testing::_))  // Call 5
              .WillOnce(DoAll(testing::SetArgumentPointee<3>(
                                    good_responses_less_one.at(i)),
                              testing::WithArgs<5>(testing::Invoke(
                  boost::bind(&test_vsl::ThreadedDoneRun, 100, 5000, _1)))));
  }

  // Call 1 - FindKNodes fails (NULL pointer)
  ASSERT_EQ(kVaultServiceFindNodesError, vsl.RemoteVaultAbleToStore(request));

  // Call 2 - FindKNodes returns kNack
  ASSERT_EQ(kVaultServiceFindNodesFailure, vsl.RemoteVaultAbleToStore(request));

  // Call 3 - FindKnodes only returns 1 node
  ASSERT_EQ(kVaultServiceFindNodesTooFew, vsl.RemoteVaultAbleToStore(request));

  // Call 4 - All OK
  ASSERT_EQ(kSuccess, vsl.RemoteVaultAbleToStore(request));

  // Call 5 - All OK - FindKNodes only returns 15 nodes, so we're contact 16
  ASSERT_EQ(kSuccess, vsl.RemoteVaultAbleToStore(request));

  // Call 6 - Fourteen responses have incorrect PMID
  ASSERT_EQ(kAccountStatusResponseError, vsl.RemoteVaultAbleToStore(request));

  // Call 7 - Fourteen responses return kNack
  ASSERT_EQ(kAccountStatusResponseFailed, vsl.RemoteVaultAbleToStore(request));

  // Call 8 - Fourteen responses don't have result set
  ASSERT_EQ(kAccountStatusResponseUninitialised,
            vsl.RemoteVaultAbleToStore(request));
}

}  // namespace maidsafe_vault

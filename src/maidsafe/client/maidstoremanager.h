/*
* ============================================================================
*
* Copyright [2009] maidsafe.net limited
*
* Description:  Manages data storage to Maidsafe network
* Version:      1.0
* Created:      2009-01-28-23.53.44
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

#ifndef MAIDSAFE_CLIENT_MAIDSTOREMANAGER_H_
#define MAIDSAFE_CLIENT_MAIDSTOREMANAGER_H_

#include <boost/filesystem.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/thread.hpp>
#include <boost/tuple/tuple.hpp>
// #include <maidsafe/transport/transportdb.h>
#include <QThreadPool>

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "maidsafe/common/accountholdersmanager.h"
#include "maidsafe/common/accountstatusmanager.h"
#include "maidsafe/common/clientbufferpackethandler.h"
#include "maidsafe/common/contactcache.h"
#include "maidsafe/client/clientutils.h"
#include "maidsafe/client/storemanager.h"
#include "maidsafe/client/storemanagertaskshandler.h"
#include "maidsafe/client/imconnectionhandler.h"
#include "maidsafe/client/imhandler.h"

namespace maidsafe { class MaidsafeStoreManager; }
namespace testpdvault {
size_t CheckStoredCopies(std::map<std::string, std::string> chunks,
                         const int &timeout_seconds,
                         boost::shared_ptr<maidsafe::MaidsafeStoreManager> sm);
}  // namespace testpdvault

namespace maidsafe {

namespace vault {
namespace test {
class RunPDVaults;
class MsmSetLocalVaultOwnedTest;
class PDVaultTest;
class PDVaultTest_FUNC_MAID_NET_StoreAndGetChunks_Test;
}  // namespace test
}  // namespace vault

namespace test {
class NetworkTest;
class CCImMessagingTest;
class CCImMessagingTest_FUNC_MAID_NET_TestImSendPresenceAndMsgs_Test;
class CCImMessagingTest_FUNC_MAID_NET_TestImRecPresenceAndSendMsgs_Test;
class CCImMessagingTest_FUNC_MAID_NET_TestMultipleImToContact_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_KeyUnique_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_ExpectAmendment_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_AddToWatchList_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_AssessUploadCounts_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_GetStoreRequests_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_ValidatePrepResp_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_SendChunkPrep_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_SendPrepCallback_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_SendChunkContent_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_RemoveFromWatchList_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_StoreNewPacket_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_StoreExistingPacket_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_LoadPacket_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_DeletePacket_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_UpdatePacket_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_GetAccountStatus_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_UpdateAccountStatus_Test;
class MaidStoreManagerTest_BEH_MAID_MSM_GetFilteredAverage_Test;
class RunPDClient;
}  // namespace test

class BufferPacketRpcs;
class ClientRpcs;
class ChunkStore;
class SessionSingleton;
struct SendChunkData;
struct StoreData;
struct DeletePacketData;
struct UpdatePacketData;
struct GenericConditionData;
struct WatchListOpData;
struct ExpectAmendmentOpData;
struct AccountStatusData;
struct AmendAccountData;
class GetChunkOpData;

class SendChunkCopyTask : public QRunnable {
 public:
  SendChunkCopyTask(boost::shared_ptr<SendChunkData> send_chunk_data,
                    MaidsafeStoreManager *msm)
      : send_chunk_data_(send_chunk_data),
        msm_(msm) {}
  void run();
 private:
  SendChunkCopyTask &operator=(const SendChunkCopyTask&);
  SendChunkCopyTask(const SendChunkCopyTask&);
  boost::shared_ptr<SendChunkData> send_chunk_data_;
  MaidsafeStoreManager *msm_;
};

class StorePacketTask : public QRunnable {
 public:
  StorePacketTask(boost::shared_ptr<StoreData> store_data,
                  MaidsafeStoreManager *msm)
      : store_data_(store_data),
        msm_(msm) {}
  void run();
 private:
  StorePacketTask &operator=(const StorePacketTask&);
  StorePacketTask(const StorePacketTask&);
  boost::shared_ptr<StoreData> store_data_;
  MaidsafeStoreManager *msm_;
};

class DeletePacketTask : public QRunnable {
 public:
  DeletePacketTask(boost::shared_ptr<DeletePacketData> delete_data,
                   MaidsafeStoreManager *msm)
      : delete_data_(delete_data),
        msm_(msm) {}
  void run();
 private:
  DeletePacketTask &operator=(const DeletePacketTask&);
  DeletePacketTask(const DeletePacketTask&);
  boost::shared_ptr<DeletePacketData> delete_data_;
  MaidsafeStoreManager *msm_;
};

class UpdatePacketTask : public QRunnable {
 public:
  UpdatePacketTask(boost::shared_ptr<UpdatePacketData> update_data,
                   MaidsafeStoreManager *msm)
      : update_data_(update_data),
        msm_(msm) {}
  void run();
 private:
  UpdatePacketTask &operator=(const UpdatePacketTask&);
  UpdatePacketTask(const UpdatePacketTask&);
  boost::shared_ptr<UpdatePacketData> update_data_;
  MaidsafeStoreManager *msm_;
};

struct SetLocalVaultOwnedCallbackArgs {
 public:
  explicit SetLocalVaultOwnedCallbackArgs(SetLocalVaultOwnedFunctor functor)
      : cb(functor),
        response(new SetLocalVaultOwnedResponse),
        ctrl(new rpcprotocol::Controller) {}
  SetLocalVaultOwnedFunctor cb;
  SetLocalVaultOwnedResponse *response;
  rpcprotocol::Controller *ctrl;
  ~SetLocalVaultOwnedCallbackArgs() {
    delete response;
    delete ctrl;
  }
 private:
  SetLocalVaultOwnedCallbackArgs(const SetLocalVaultOwnedCallbackArgs&);
  SetLocalVaultOwnedCallbackArgs &operator=(
      const SetLocalVaultOwnedCallbackArgs&);
};

struct LocalVaultOwnedCallbackArgs {
 public:
  explicit LocalVaultOwnedCallbackArgs(LocalVaultOwnedFunctor functor)
      : cb(functor),
        response(new LocalVaultOwnedResponse),
        ctrl(new rpcprotocol::Controller) {}
  ~LocalVaultOwnedCallbackArgs() {
    delete response;
    delete ctrl;
  }
  LocalVaultOwnedFunctor cb;
  LocalVaultOwnedResponse *response;
  rpcprotocol::Controller *ctrl;
 private:
  LocalVaultOwnedCallbackArgs(const LocalVaultOwnedCallbackArgs&);
  LocalVaultOwnedCallbackArgs &operator=(const LocalVaultOwnedCallbackArgs&);
};

struct PresenceMessages {
  std::set<std::string> presence_set;
  boost::uint16_t successes;
  bool done;
  boost::mutex mutex;
  boost::condition_variable cond;
};

struct VBPMessages {
  std::set<std::string> presence_set;
  boost::uint16_t successes;
  bool done;
  boost::mutex mutex;
  boost::condition_variable cond;
};

struct BPResults {
  size_t returned_count;
  std::map<std::string, ReturnCode> *results;
  boost::mutex mutex;
  boost::condition_variable cond;
  bool finished;
  ReturnCode rc;
};

class MaidsafeStoreManager : public StoreManagerInterface {
 public:
  MaidsafeStoreManager(boost::shared_ptr<ChunkStore> cstore, boost::uint8_t k);
  virtual ~MaidsafeStoreManager() {}
  void Init(VoidFuncOneInt callback,
            const boost::uint16_t &port);
  void Init(VoidFuncOneInt callback,
            const boost::filesystem::path &kad_config,
            const boost::uint16_t &port);
  void SetPmid(const std::string &pmid_name);
  void Close(VoidFuncOneInt callback, bool cancel_pending_ops);
  void CleanUpTransport();
  void StopRvPing() { transport_handler_.StopPingRendezvous(); }
  bool KeyUnique(const std::string &key, bool check_local);
  void KeyUnique(const std::string &key,
                 bool check_local,
                 const VoidFuncOneInt &cb);
  bool NotDoneWithUploading();
  // Initialises the chunk storing process. Non-blocking, returns after
  // sending the AddToWatchListRequest.
  int StoreChunk(const std::string &chunk_name,
                 DirType dir_type,
                 const std::string &msid);
  // Adds the packet to the priority store queue for uploading as a Kad k,v pair
  void StorePacket(const std::string &packet_name,
                   const std::string &value,
                   passport::PacketType system_packet_type,
                   DirType dir_type,
                   const std::string &msid,
                   const VoidFuncOneInt &cb);
  int LoadChunk(const std::string &chunk_name, std::string *data);
  // Blocking call which loads all values stored under the packet name
  int LoadPacket(const std::string &packet_name,
                 std::vector<std::string> *results);
  // Non-blocking call which loads all values stored under the packet name
  void LoadPacket(const std::string &packet_name, const LoadPacketFunctor &lpf);
  int DeleteChunk(const std::string &chunk_name,
                  const boost::uint64_t &chunk_size,
                  DirType dir_type,
                  const std::string &msid);
  // Deletes all values for the specified key
  void DeletePacket(const std::string &packet_name,
                    const std::vector<std::string> values,
                    passport::PacketType system_packet_type,
                    DirType dir_type,
                    const std::string &msid,
                    const VoidFuncOneInt &cb);
  virtual void UpdatePacket(const std::string &packet_name,
                            const std::string &old_value,
                            const std::string &new_value,
                            passport::PacketType system_packet_type,
                            DirType dir_type,
                            const std::string &msid,
                            const VoidFuncOneInt &cb);

  void GetAccountStatus(boost::uint64_t *space_offered,
                        boost::uint64_t *space_given,
                        boost::uint64_t *space_taken);
  // Buffer packet
  virtual int CreateBP();
  virtual int LoadBPMessages(std::list<ValidatedBufferPacketMessage> *messages);
  virtual int ModifyBPInfo(const std::string &info);
  virtual int SendMessage(const std::vector<std::string> &receivers,
                           const std::string &message,
                           const MessageType &type,
                           std::map<std::string, ReturnCode> *add_results);
  virtual int LoadBPPresence(std::list<LivePresence> *messages);
  virtual int AddBPPresence(
      const std::vector<std::string> &receivers,
      std::map<std::string, ReturnCode> *add_results);

  // Vault
  void PollVaultInfo(kad::VoidFunctorOneString cb);
  bool VaultContactInfo(kad::Contact *contact);
  void SetLocalVaultOwned(const std::string &priv_key,
                          const std::string &pub_key,
                          const std::string &signed_pub_key,
                          const boost::uint32_t &port,
                          const std::string &vault_dir,
                          const boost::uint64_t &space,
                          const SetLocalVaultOwnedFunctor &functor);
  void LocalVaultOwned(const LocalVaultOwnedFunctor &functor);
  virtual int CreateAccount(const boost::uint64_t &space);

  // Instant messaging send online message
  bool SendPresence(const std::string &contactname);
  void SendLogOutMessage(const std::string &contactname);
  void SetSessionEndPoint();
  void SetInstantMessageNotifier(IMNotifier on_msg,
                                 IMStatusNotifier status_notifier);
 private:
  MaidsafeStoreManager &operator=(const MaidsafeStoreManager&);
  MaidsafeStoreManager(const MaidsafeStoreManager&);
  friend void SendChunkCopyTask::run();
  friend void StorePacketTask::run();
  friend void DeletePacketTask::run();
  friend void UpdatePacketTask::run();
  friend size_t testpdvault::CheckStoredCopies(
      std::map<std::string, std::string> chunks,
      const int &timeout, boost::shared_ptr<MaidsafeStoreManager> sm);
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_KeyUnique_Test;
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_ExpectAmendment_Test;
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_AddToWatchList_Test;
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_AssessUploadCounts_Test;
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_GetStoreRequests_Test;
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_ValidatePrepResp_Test;
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_SendChunkPrep_Test;
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_SendPrepCallback_Test;
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_SendChunkContent_Test;
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_RemoveFromWatchList_Test;
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_StoreNewPacket_Test;
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_StoreExistingPacket_Test;
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_LoadPacket_Test;
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_DeletePacket_Test;
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_UpdatePacket_Test;
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_GetAccountStatus_Test;
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_UpdateAccountStatus_Test;
  friend class test::MaidStoreManagerTest_BEH_MAID_MSM_GetFilteredAverage_Test;
  friend class vault::test::MsmSetLocalVaultOwnedTest;
  friend class vault::test::PDVaultTest;
  friend class vault::test::PDVaultTest_FUNC_MAID_NET_StoreAndGetChunks_Test;
  friend class vault::test::RunPDVaults;
  friend class test::RunPDClient;
  friend class test::NetworkTest;
  friend class test::CCImMessagingTest;
  friend class
      test::CCImMessagingTest_FUNC_MAID_NET_TestImSendPresenceAndMsgs_Test;
  friend class
      test::CCImMessagingTest_FUNC_MAID_NET_TestImRecPresenceAndSendMsgs_Test;
  friend class
      test::CCImMessagingTest_FUNC_MAID_NET_TestMultipleImToContact_Test;
  void AccountHoldersCallback(const ReturnCode &result,
                              const std::vector<kad::Contact> &holders);
  // Check the inputs to the public methods are valid
  ReturnCode ValidateInputs(const std::string &name,
                            const passport::PacketType &packet_type,
                            const DirType &dir_type);
  void AddStorePacketTask(const StoreData &store_data,
                          bool is_mutable,
                          int *return_value,
                          GenericConditionData *generic_cond_data);
  // Finds Chunk Info Holders, continued in further stages.
  virtual int AddToWatchList(boost::shared_ptr<StoreData> store_data);
  // Sends ExpectAmendment requests.
  void AddToWatchListStageTwo(
      const ReturnCode &result,
      const std::vector<kad::Contact> &chunk_info_holders,
      boost::shared_ptr<WatchListOpData> data);
  // Sends AddToWatchList requests.
  void AddToWatchListStageThree(const ReturnCode &result,
                                boost::shared_ptr<WatchListOpData> data);
  // Assesses response and if consensus of required chunk upload copies is
  // achieved, begins new SendChunkCopyTask(s) if needed.
  void AddToWatchListStageFour(boost::uint16_t index,
                               boost::shared_ptr<WatchListOpData> data);
  // Prepares and executes ExpectAmendment RPCs.
  void ExpectAmendment(const std::string &chunkname,
                       const AmendAccountRequest::Amendment &amendment_type,
                       const std::string &pmid,
                       const std::string &public_key,
                       const std::string &public_key_signature,
                       const std::string &private_key,
                       DirType dir_type,
                       const std::vector<kad::Contact> &chunk_info_holders,
                       const VoidFuncOneInt &callback);
  // Assesses results of ExpectAmendment RPCs.
  void ExpectAmendmentCallback(size_t index,
                               boost::shared_ptr<ExpectAmendmentOpData> data);
  // Assesses AddToWatchListResponses for consensus of required chunk upload
  // copies.  Returns < 0 if no consensus.  data->mutex should already be locked
  // by method calling this one for duration of this function.
  int AssessUploadCounts(boost::shared_ptr<WatchListOpData> data);
  // Finds Chunk Info Holders, continued in further stages.
  virtual int RemoveFromWatchList(boost::shared_ptr<StoreData> store_data);
  // Sends ExpectAmendment requests.
  void RemoveFromWatchListStageTwo(
      const ReturnCode &result,
      const std::vector<kad::Contact> &chunk_info_holders,
      boost::shared_ptr<WatchListOpData> data);
  // Sends RemoveFromWatchList requests.
  void RemoveFromWatchListStageThree(const ReturnCode &result,
                                     boost::shared_ptr<WatchListOpData> data);
  // Assesses responses.
  void RemoveFromWatchListStageFour(boost::uint16_t index,
                                    boost::shared_ptr<WatchListOpData> data);
  // Contact the chunk info holders to retrieve the list of chunk holders
  bool GetChunkReferences(const std::string &chunk_name,
                          const std::vector<kad::Contact> &chunk_info_holders,
                          std::vector<std::string> *references);
  void UpdateAccountStatus();
  void UpdateAccountStatusStageTwo(size_t index,
                                   boost::shared_ptr<AccountStatusData> data);
  void NotifyTaskHandlerOfAccountAmendments(
      const AccountStatusResponse &account_status_response);
  // Calculates the mean of only the values within sqrt(2) std devs from mean
  // TODO(Team#) move to central place for global usage?
  static void GetFilteredAverage(const std::vector<boost::uint64_t> &values,
                                 boost::uint64_t *average,
                                 size_t *n);
  // Blocks until either we're online (returns true) or until task is cancelled
  // or finished (returns false)
  virtual bool WaitForOnline(const TaskId &task_id);
  // Set up the requests needed to perform the store RPCs.
  int GetStoreRequests(boost::shared_ptr<SendChunkData> send_chunk_data);
  // Set up the requests needed to perform the AddToWatchList RPCs.
  int GetAddToWatchListRequests(
      boost::shared_ptr<StoreData> store_data,
      const std::vector<kad::Contact> &recipients,
      std::vector<AddToWatchListRequest> *add_to_watch_list_requests);
  // Set up the requests needed to perform the RemoveFromWatchList RPCs.
  int GetRemoveFromWatchListRequests(
      boost::shared_ptr<StoreData> store_data,
      const std::vector<kad::Contact> &recipients,
      std::vector<RemoveFromWatchListRequest> *remove_from_watch_list_requests);
  // Get the request signature for a chunk / packet.
  void GetRequestSignature(const std::string &name,
                           const DirType dir_type,
                           const std::string &recipient_id,
                           const std::string &public_key,
                           const std::string &public_key_signature,
                           const std::string &private_key,
                           std::string *request_signature);
  // Get the request signature for a chunk / packet store task.
  void GetRequestSignature(boost::shared_ptr<StoreData> store_data,
                           const std::string &recipient_id,
                           std::string *request_signature);
  // Called on completion of the StoreChunk root task
  void StoreChunkTaskCallback(const TaskId &task_id,
                              const ReturnCode &result,
                              const boost::uint64_t &reserved_space);
  // Called on completion of the root task's child tasks, for debugging only
  void DebugSubTaskCallback(const TaskId &task_id,
                            const ReturnCode &result,
                            const std::string &task_info);
  // Called on completion of the DeleteChunk root task
  void DeleteChunkTaskCallback(const TaskId &task_id,
                               const ReturnCode &result);
  // Called on completion of an AddToWatchList child task
  void AddToWatchListTaskCallback(const ReturnCode &result,
                                  boost::shared_ptr<StoreData> store_data,
                                  const TaskId &amendment_task_id);
  void RemoveFromWatchListTaskCallback(const ReturnCode &result,
                                       boost::shared_ptr<StoreData> store_data);
  // Start process of storing a single copy of an individual chunk onto the net.
  void StoreChunkCopy(boost::shared_ptr<StoreData> store_data);
  void StoreChunkCopy(boost::shared_ptr<StoreData> store_data,
                      const kad::Contact &force_peer);
  // Initialise chunk storing preparation.
  void DoStorePrep(boost::shared_ptr<SendChunkData> send_chunk_data);
  void ChunkCopyPrepTaskCallback(
      const ReturnCode &result,
      boost::shared_ptr<SendChunkData> send_chunk_data);
  void StorePrepCallback(TaskId prep_task_id,
                         boost::shared_ptr<SendChunkData> send_chunk_data);
  virtual int ValidatePrepResponse(
      const std::string &peer_node_id,
      const SignedSize &request_signed_size,
      const StorePrepResponse *store_prep_response);
  // Send the actual data content to the peer.
  void DoStoreChunk(boost::shared_ptr<SendChunkData> send_chunk_data);
  void ChunkCopyDataTaskCallback(
      const ReturnCode &result,
      boost::shared_ptr<SendChunkData> send_chunk_data);
  void StoreChunkCallback(TaskId data_task_id,
                          boost::shared_ptr<SendChunkData> send_chunk_data);
  void LoadChunk_FindCB(const std::string &result,
                        boost::shared_ptr<GetChunkOpData> data);
  void LoadChunk_RefsCB(size_t rsp_idx,
                        boost::shared_ptr<GetChunkOpData> data);
  void LoadChunk_HolderCB(const ReturnCode &result,
                          const kad::Contact &chunk_holder,
                          const std::string &pmid,
                          boost::shared_ptr<GetChunkOpData> data);
  void LoadChunk_CheckCB(std::pair<std::string, size_t> params,
                         boost::shared_ptr<GetChunkOpData> data);
  // Get a chunk's content from a specific peer.
  int GetChunk(const std::string &chunk_name,
               const kad::Contact &chunk_holder,
               std::string *data);
  void GetChunkCallback(
      bool *done,
      std::pair<boost::mutex*, boost::condition_variable*> sync);
  // Callback for blocking version of LoadPacket
  void LoadPacketCallback(const std::vector<std::string> values_in,
                          const int &result_in,
                          boost::mutex *mutex,
                          boost::condition_variable *cond_var,
                          std::vector<std::string> *values_out,
                          int *result_out);
  // Callback for non-blocking version of LoadPacket
  void LoadPacketCallback(const std::string &packet_name,
                          const int &attempt,
                          const std::string &ser_response,
                          const LoadPacketFunctor &lpf);
  // Callback for blocking version of KeyUnique
  void KeyUniqueCallback(const ReturnCode &result_in,
                         boost::mutex *mutex,
                         boost::condition_variable *cond_var,
                         bool *result_out,
                         bool *called_back);
  // Callback for non-blocking version of KeyUnique
  void KeyUniqueCallback(const std::string &ser_response,
                         const VoidFuncOneInt &cb);
  // Store an individual packet to the network as a kademlia value.
  virtual void SendPacket(boost::shared_ptr<StoreData> store_data);
  void SendPacketCallback(const std::string &ser_kad_store_result,
                          boost::shared_ptr<StoreData> store_data);
  std::string GetValueFromSignedValue(
      const std::string &serialised_signed_value);
  virtual void DeletePacketFromNet(
      boost::shared_ptr<DeletePacketData> delete_data);
  void DeletePacketCallback(const std::string &ser_kad_delete_result,
                            boost::shared_ptr<DeletePacketData> delete_data);
  virtual void UpdatePacketOnNetwork(
      boost::shared_ptr<UpdatePacketData> update_data);
  void UpdatePacketCallback(const std::string &ser_kad_update_result,
                            boost::shared_ptr<UpdatePacketData> delete_data);
  void DoNothingCallback(const std::string&) {}
  void PollVaultInfoCallback(const VaultStatusResponse *response,
                             kad::VoidFunctorOneString cb);
  void SetLocalVaultOwnedCallback(
      boost::shared_ptr<SetLocalVaultOwnedCallbackArgs> callback_args);
  void LocalVaultOwnedCallback(
      boost::shared_ptr<LocalVaultOwnedCallbackArgs> callback_args);
  void AmendAccountCallback(size_t index,
                            boost::shared_ptr<AmendAccountData> data);
  BPInputParameters GetBPInputParameters();
  void ModifyBpCallback(const ReturnCode &rc,
                        boost::shared_ptr<BPResults> pm);
  void AddToBpCallback(const ReturnCode &rc,
                       const std::string &receiver,
                       boost::shared_ptr<BPResults> results);
  void LoadMessagesCallback(const ReturnCode &res,
                            const std::list<ValidatedBufferPacketMessage> &msgs,
                            bool b,
                            boost::shared_ptr<VBPMessages> vbpms);
  void LoadPresenceCallback(const ReturnCode &res,
                            const std::list<std::string> &pres,
                            bool b,
                            boost::shared_ptr<PresenceMessages> pm);
  std::string ValidatePresence(const std::string &ser_presence);

  // Instant messaging related functions
  void CloseConnection(const std::string &contactname);
  void OnMessage(const std::string &msg);
  void OnNewConnection(const boost::int16_t &trans_id,
      const boost::uint32_t &conn_id, const std::string &msg);
  bool SendIM(const std::string &msg, const std::string &contactname);

  const boost::uint8_t K_;
  const boost::uint16_t kUpperThreshold_;
  const boost::uint16_t kLowerThreshold_;
  transport::TransportUDT transport_;
  // transport::TransportDb transport_;
  transport::TransportHandler transport_handler_;
  rpcprotocol::ChannelManager channel_manager_;
  boost::shared_ptr<ClientRpcs> client_rpcs_;
  boost::shared_ptr<KadOps> kad_ops_;
  SessionSingleton *ss_;
  ClientUtils client_utils_;
  StoreManagerTasksHandler tasks_handler_;
  boost::shared_ptr<ChunkStore> client_chunkstore_;
  QThreadPool chunk_thread_pool_, packet_thread_pool_;
  boost::shared_ptr<BufferPacketRpcs> bprpcs_;
  ClientBufferPacketHandler cbph_;
  const int kChunkMaxThreadCount_;
  const int kPacketMaxThreadCount_;
  IMNotifier im_notifier_;
  IMStatusNotifier im_status_notifier_;
  IMConnectionHandler im_conn_hdler_;
  IMHandler im_handler_;
  ContactCache own_vault_;
  AccountHoldersManager account_holders_manager_;
  AccountStatusManager account_status_manager_;
  boost::shared_ptr<AccountStatusData> account_status_update_data_;
};

}  // namespace maidsafe

#endif  // MAIDSAFE_CLIENT_MAIDSTOREMANAGER_H_

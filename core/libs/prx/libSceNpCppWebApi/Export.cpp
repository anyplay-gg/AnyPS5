#include "prx/libSceNpCppWebApi/include/Api.hpp"
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libSceNetCtl/include/NetBridge.hpp"

static constexpr int SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN = static_cast<int>(0x80552907);
static constexpr int TransactionNotStarted = static_cast<int>(0x80553508);

// Offline PSN: request entry points (API calls, transaction start/readData, factories) fail with
// SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN, the constant shadPS4/fpPS4 document for this code; parameter
// bookkeeping succeeds; response accessors stay unimplemented because a failed request never
// produces a response.
extern "C" {

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V111UserFactory6createEPNS1_6Common10LibContextEPNS5_12IntrusivePtrINS3_4UserEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V121GetRankingRequestBody18setStartSerialRankERKi() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V121GetRankingRequestBody8setGroupERKNS3_5GroupE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V121GetRankingRequestBody8setLimitERKi() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V121GetRankingRequestBody8setUsersERKNS1_6Common6VectorINS5_12IntrusivePtrINS3_4UserEEEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V121GetRankingRequestBody9setOffsetERKi() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V122GetRankingResponseBody10getEntriesEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V122RecordScoreRequestBody10setCommentEPKc() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V122RecordScoreRequestBody12setSmallDataEPKvm() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V122RecordScoreRequestBody15setNeedsTmpRankERKb() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V122RecordScoreRequestBody19setComparedDateTimeERK10SceRtcTick() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V122RecordScoreRequestBody7setPcIdERKi() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V128GetRankingRequestBodyFactory6createEPNS1_6Common10LibContextEPNS5_12IntrusivePtrINS3_21GetRankingRequestBodyEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V129RecordScoreRequestBodyFactory6createEPNS1_6Common10LibContextElPNS5_12IntrusivePtrINS3_22RecordScoreRequestBodyEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V14User12setAccountIdERKm() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V14User7setPcIdERKi() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi10getRankingEiRKNS4_21ParameterToGetRankingERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_22GetRankingResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi21ParameterToGetRanking10initializeEPNS1_6Common10LibContextEi() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi21ParameterToGetRanking24setgetRankingRequestBodyENS1_6Common12IntrusivePtrINS3_21GetRankingRequestBodyEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi21ParameterToGetRanking9terminateEv() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi21ParameterToGetRankingC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi21ParameterToGetRankingD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi22getLargeDataByObjectIdEiRKNS4_33ParameterToGetLargeDataByObjectIdERNS1_6Common21DownStreamTransactionINS8_12IntrusivePtrINS4_37GetLargeDataByObjectIdResponseHeadersEEEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi33ParameterToGetLargeDataByObjectId10initializeEPNS1_6Common10LibContextEPKc() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi33ParameterToGetLargeDataByObjectId9terminateEv() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi33ParameterToGetLargeDataByObjectIdC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V17ViewApi33ParameterToGetLargeDataByObjectIdD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19BoardsApi18getBoardDefinitionEiRKNS4_29ParameterToGetBoardDefinitionERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_30GetBoardDefinitionResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19BoardsApi29ParameterToGetBoardDefinition10initializeEPNS1_6Common10LibContextEi() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19BoardsApi29ParameterToGetBoardDefinition9terminateEv() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19BoardsApi29ParameterToGetBoardDefinitionC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19BoardsApi29ParameterToGetBoardDefinitionD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi11recordScoreEiRKNS4_22ParameterToRecordScoreERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_23RecordScoreResponseBodyEEENSA_INS4_26RecordScoreResponseHeadersEEEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi15recordLargeDataEiRKNS4_26ParameterToRecordLargeDataERNS1_6Common19UpStreamTransactionINS8_12IntrusivePtrINS3_27RecordLargeDataResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi22ParameterToRecordScore10initializeEPNS1_6Common10LibContextEiNS6_12IntrusivePtrINS3_22RecordScoreRequestBodyEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi22ParameterToRecordScore22setxPsnAtomicOperationENS5_19XPsnAtomicOperationE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi22ParameterToRecordScore9terminateEv() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi22ParameterToRecordScoreC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi22ParameterToRecordScoreD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi26ParameterToRecordLargeData10initializeEPNS1_6Common10LibContextEiNS5_19XPsnAtomicOperationEPKc() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi26ParameterToRecordLargeData9terminateEv() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi26ParameterToRecordLargeDataC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi12Leaderboards2V19RecordApi26ParameterToRecordLargeDataD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi12downloadDataEiRKNS4_23ParameterToDownloadDataERNS1_6Common21DownStreamTransactionINS8_12IntrusivePtrINS4_27DownloadDataResponseHeadersEEEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

static constexpr int ParameterNotInitialized = static_cast<int>(0x80553502);
static constexpr int ParameterInvalidArgument = static_cast<int>(0x80553503);
static constexpr int ParameterAlreadyInitialized = static_cast<int>(0x8055350f);
static constexpr int ParameterTerminatedUninitialized = static_cast<int>(0x80553510);
static constexpr int ParameterBodyMissing = static_cast<int>(0x80553512);

struct ParameterObject {
    void* base;
    const void* implementation;
};

static constexpr unsigned char ParameterImplementation = 0;

static void ConstructParameter(void* self) {
    *static_cast<ParameterObject*>(self) = {};
}

static int InitializeParameter(void* self, const std::uint64_t* context, const bool argumentsPresent) {
    if (context == nullptr) return ParameterInvalidArgument;
    if (*context == 0) return ParameterNotInitialized;
    auto* parameter = static_cast<ParameterObject*>(self);
    if (parameter->implementation != nullptr) return ParameterAlreadyInitialized;
    if (!argumentsPresent) return ParameterInvalidArgument;
    parameter->implementation = &ParameterImplementation;
    return 0;
}

static int SetParameterText(const void* self, const char* text) {
    if (static_cast<const ParameterObject*>(self)->implementation == nullptr) return ParameterNotInitialized;
    return text == nullptr ? ParameterInvalidArgument : 0;
}

static int TerminateParameter(void* self) {
    auto* parameter = static_cast<ParameterObject*>(self);
    if (parameter->implementation == nullptr) return ParameterTerminatedUninitialized;
    parameter->implementation = nullptr;
    return 0;
}

static int InitializeParameterWithBody(void* self, const std::uint64_t* context, const bool textsPresent, const IntrusivePtr* body) {
    const int result = InitializeParameter(self, context, textsPresent);
    if (result != 0 || body->object != nullptr) return result;
    static_cast<ParameterObject*>(self)->implementation = nullptr;
    return ParameterBodyMissing;
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadData10initializeEPNS1_6Common10LibContextEPKci(void* self, const std::uint64_t* context, const char* text, int value) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)value;
    return netCall.Ret(InitializeParameter(self, context, text != nullptr));
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadData9terminateEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TerminateParameter(self));
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadDataC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ConstructParameter(self);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadDataD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common10InitParamsC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common10InitParamsD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common10LibContextC1Ev(uint64_t* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = 0;
    return netCall.Ret(0);
}

// Initialization succeeds as on an offline console (see libSceJson2); individual requests fail.
int APS5_VABI _ZN3sce2Np9CppWebApi6Common10initializeERKNS2_10InitParamsERNS2_10LibContextE(const void* params, uint64_t* context) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)params;
    if (context) *context = 1;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V122GetRankingResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V122GetRankingResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEEENS4_INS6_9RecordApi26RecordScoreResponseHeadersEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEEENS4_INS6_9RecordApi26RecordScoreResponseHeadersEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V130GetBoardDefinitionResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V130GetBoardDefinitionResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

static_assert(sizeof(IntrusivePtr) == 0x18);

static constexpr std::size_t ReferenceCountOffset = 0;
static constexpr std::size_t VectorReferenceCountOffset = 0x30;

static std::atomic_ref<std::int32_t> ReferenceCount(void* object, const std::size_t offset) {
    return std::atomic_ref<std::int32_t>(*reinterpret_cast<std::int32_t*>(static_cast<std::uint8_t*>(object) + offset));
}

static void CopyIntrusivePtr(IntrusivePtr* self, const IntrusivePtr* source, const std::size_t referenceCountOffset) {
    *self = *source;
    if (self->object) ReferenceCount(self->object, referenceCountOffset).fetch_add(1);
}

static void ReleaseIntrusivePtr(IntrusivePtr* self, const std::size_t referenceCountOffset, const char* function) {
    if (!self->object) return;
    if (ReferenceCount(self->object, referenceCountOffset).fetch_sub(1) != 1) return;
    if (!self->deleter) NotImplemented_nid_no_patch(function);
    self->deleter(self->object);
    self->object = nullptr;
    self->deleter = nullptr;
}

static void AssignIntrusivePtr(IntrusivePtr* self, const IntrusivePtr* source, const std::size_t referenceCountOffset, const char* function) {
    if (self->object == source->object) return;
    ReleaseIntrusivePtr(self, referenceCountOffset, function);
    CopyIntrusivePtr(self, source, referenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V121GetRankingRequestBodyEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V121GetRankingRequestBodyEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V121GetRankingRequestBodyEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, ReferenceCountOffset, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V122GetRankingResponseBodyEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V122GetRankingResponseBodyEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, ReferenceCountOffset, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V122RecordScoreRequestBodyEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V122RecordScoreRequestBodyEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V122RecordScoreRequestBodyEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, ReferenceCountOffset, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, ReferenceCountOffset, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V130GetBoardDefinitionResponseBodyEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V130GetBoardDefinitionResponseBodyEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, ReferenceCountOffset, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V14UserEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V14UserEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, ReferenceCountOffset, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V19RecordApi26RecordScoreResponseHeadersEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V19RecordApi26RecordScoreResponseHeadersEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, ReferenceCountOffset, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_12Leaderboards2V15EntryEEEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, VectorReferenceCountOffset, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEED2Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_12Leaderboards2V122GetRankingResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_12Leaderboards2V122GetRankingResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEEENS4_INS6_9RecordApi26RecordScoreResponseHeadersEEEE5startEPNS2_10LibContextE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEEENS4_INS6_9RecordApi26RecordScoreResponseHeadersEEEE6finishEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_12Leaderboards2V127RecordLargeDataResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_12Leaderboards2V130GetBoardDefinitionResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_12Leaderboards2V130GetBoardDefinitionResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common19UpStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V127RecordLargeDataResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextEm() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common19UpStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V127RecordLargeDataResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE8sendDataEPKvm() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common19UpStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V127RecordLargeDataResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common19UpStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V127RecordLargeDataResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V17ViewApi37GetLargeDataByObjectIdResponseHeadersEEEE5startEPNS2_10LibContextE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V17ViewApi37GetLargeDataByObjectIdResponseHeadersEEEE6finishEv() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V17ViewApi37GetLargeDataByObjectIdResponseHeadersEEEE8readDataEPcm() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V17ViewApi37GetLargeDataByObjectIdResponseHeadersEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_12Leaderboards2V17ViewApi37GetLargeDataByObjectIdResponseHeadersEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V17DataApi27DownloadDataResponseHeadersEEEE5startEPNS2_10LibContextE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V17DataApi27DownloadDataResponseHeadersEEEE6finishEv() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V17DataApi27DownloadDataResponseHeadersEEEE8readDataEPcm() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V17DataApi27DownloadDataResponseHeadersEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common21DownStreamTransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V17DataApi27DownloadDataResponseHeadersEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

static_assert(sizeof(CommonString) == 0x20 && offsetof(CommonString, buffer) == 0x08);
static_assert(offsetof(CommonString, bufferSize) == 0x10 && offsetof(CommonString, libContext) == 0x18);

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6StringC1EPNS2_10LibContextE(CommonString* self, void* libContext) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    self->referenceCount = 0;
    self->buffer = nullptr;
    self->bufferSize = 0;
    self->libContext = libContext;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6StringD1Ev(CommonString* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    std::free(self->buffer);
    self->buffer = nullptr;
    self->bufferSize = 0;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V14UserEEEE8pushBackERKS8_() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V14UserEEEEC1EPNS2_10LibContextE(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V14UserEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEE3endEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEE5beginEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEEppEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common9terminateERNS2_10LibContextE(uint64_t* context) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *context = 0;
    return netCall.Ret(0);
}

const void* APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V122GetRankingResponseBody22getLastUpdatedDateTimeEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V123RecordScoreResponseBody10getTmpRankEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

std::uint32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V123RecordScoreResponseBody16getTmpSerialRankEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody11getSortModeEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

std::uint32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody13getEntryLimitEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody13getUpdateModeEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody13sortModeIsSetEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody15entryLimitIsSetEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody15updateModeIsSetEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody16getMaxScoreLimitEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody16getMinScoreLimitEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody18maxScoreLimitIsSetEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody18minScoreLimitIsSetEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

std::uint32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody20getLargeDataNumLimitEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

std::size_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody21getLargeDataSizeLimitEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody22largeDataNumLimitIsSetEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V130GetBoardDefinitionResponseBody23largeDataSizeLimitIsSetEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry10getCommentEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry11getObjectIdEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry11getOnlineIdEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry12commentIsSetEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

std::uint64_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry12getAccountIdEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

const void* APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry12getSmallDataEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

std::uint32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry13getSerialRankEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry13objectIdIsSetEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

std::uint32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry14getHighestRankEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry14smallDataIsSetEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

std::uint32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry20getHighestSerialRankEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry7getPcIdEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

std::uint32_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry7getRankEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V15Entry8getScoreEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi12Leaderboards2V19RecordApi26RecordScoreResponseHeaders24getXPsnAtomicOperationIdEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V122GetRankingResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEEENS4_INS6_9RecordApi26RecordScoreResponseHeadersEEEE11getResponseERS8_() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_12Leaderboards2V130GetBoardDefinitionResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V121GetRankingRequestBodyEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V122GetRankingResponseBodyEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V122RecordScoreRequestBodyEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V130GetBoardDefinitionResponseBodyEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V14UserEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V15EntryEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_12Leaderboards2V19RecordApi26RecordScoreResponseHeadersEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6BinaryEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_12Leaderboards2V15EntryEEEEEEdeEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_12Leaderboards2V123RecordScoreResponseBodyEEENS4_INS6_9RecordApi26RecordScoreResponseHeadersEEEE18getResponseHeadersERSB_() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

std::size_t APS5_VABI _ZNK3sce2Np9CppWebApi6Common6Binary4sizeEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

const void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common6Binary9getBinaryEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi6Common6String5c_strEv(const CommonString* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->buffer != nullptr ? self->buffer : "");
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEEdeEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_12Leaderboards2V15EntryEEEEneERKS9_(const void* self, const void* other) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    (void)other;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

// Shared vector bodies. A Vector handle is an IntrusivePtr whose object is a body;
// bodies adopted from a title through the pointer/deleter constructor keep their
// 32-bit reference count at 0x30, so bodies created here do the same.
static constexpr std::uint64_t VectorBodyMagic = 0x434e507056656342ULL;
struct VectorBody {
    std::uint64_t magic;
    std::uint8_t reserved[0x28];
    std::int32_t referenceCount;
    std::int32_t elementKind;  // 1 = IntrusivePtr elements, 2 = String elements
    void* elements;
    std::uint64_t size;
    std::uint64_t capacity;
};
static_assert(sizeof(VectorBody) == 0x50 && offsetof(VectorBody, referenceCount) == 0x30);

static void APS5_VABI DestroyVectorBody(void* object) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    auto* body = static_cast<VectorBody*>(object);
    if (body->elementKind == 1) {
        auto* elements = static_cast<IntrusivePtr*>(body->elements);
        for (std::uint64_t index = 0; index < body->size; ++index) {
            ReleaseIntrusivePtr(&elements[index], 0x00, __func__);
        }
    } else if (body->elementKind == 2) {
        auto* elements = static_cast<CommonString*>(body->elements);
        for (std::uint64_t index = 0; index < body->size; ++index) {
            std::free(elements[index].buffer);
        }
    }
    std::free(body->elements);
    std::free(body);
}

static VectorBody* VectorBodyFor(IntrusivePtr* self, const std::int32_t elementKind, const char* function) {
    auto* body = static_cast<VectorBody*>(self->object);
    if (body == nullptr) {
        body = static_cast<VectorBody*>(std::calloc(1, sizeof(VectorBody)));
        if (body == nullptr) return nullptr;
        body->magic = VectorBodyMagic;
        body->referenceCount = 1;
        body->elementKind = elementKind;
        self->object = body;
        self->deleter = &DestroyVectorBody;
        return body;
    }
    if (body->magic != VectorBodyMagic || body->elementKind != elementKind) {
        // appending to a title-provided body needs its element layout, which is unknown
        NotImplemented_nid_no_patch(function);
        return nullptr;
    }
    return body;
}

static bool VectorGrow(VectorBody* body, const std::size_t elementSize) {
    if (body->size < body->capacity) return true;
    const std::uint64_t capacity = body->capacity != 0 ? body->capacity * 2 : 4;
    void* elements = std::realloc(body->elements, static_cast<std::size_t>(capacity) * elementSize);
    if (elements == nullptr) return false;
    body->elements = elements;
    body->capacity = capacity;
    return true;
}

static std::uint64_t HostVectorSize(const void* self, const char* function) {
    const auto* body = static_cast<const VectorBody*>(static_cast<const IntrusivePtr*>(self)->object);
    if (body == nullptr) return 0;
    if (body->magic != VectorBodyMagic) NotImplemented_nid_no_patch(function);
    return body->size;
}

static IntrusivePtr* HostVectorElement(void* self, const std::uint64_t index, const char* function) {
    auto* body = static_cast<VectorBody*>(static_cast<IntrusivePtr*>(self)->object);
    if (body == nullptr || body->magic != VectorBodyMagic || body->elementKind != 1) NotImplemented_nid_no_patch(function);
    return static_cast<IntrusivePtr*>(body->elements) + index;
}

static int VectorPushBackIntrusivePtr(IntrusivePtr* self, const IntrusivePtr* element) {
    VectorBody* body = VectorBodyFor(self, 1, __func__);
    if (body == nullptr) return -1;
    if (!VectorGrow(body, sizeof(IntrusivePtr))) return -1;
    auto* elements = static_cast<IntrusivePtr*>(body->elements);
    elements[body->size] = *element;
    if (element->object != nullptr) ReferenceCount(element->object, ReferenceCountOffset).fetch_add(1);
    body->size += 1;
    return 0;
}

static int VectorPushBackString(IntrusivePtr* self, const CommonString* element) {
    VectorBody* body = VectorBodyFor(self, 2, __func__);
    if (body == nullptr) return -1;
    if (!VectorGrow(body, sizeof(CommonString))) return -1;
    CommonString copy = *element;
    copy.referenceCount = 0;
    copy.buffer = nullptr;
    copy.bufferSize = 0;
    if (element->buffer != nullptr) {
        const std::size_t length = std::strlen(element->buffer) + 1;
        copy.buffer = static_cast<char*>(std::malloc(length));
        if (copy.buffer == nullptr) return -1;
        std::memcpy(copy.buffer, element->buffer, length);
        copy.bufferSize = length;
    }
    static_cast<CommonString*>(body->elements)[body->size] = copy;
    body->size += 1;
    return 0;
}
// --------------------------------------------------------------------------
// SessionManager2V1 and extra Common symbols, from a title dump whose modules lack
// libSceNpCppWebApi.prx.
// --------------------------------------------------------------------------

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V115LocalizedStringEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, 0x00);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V120RequestPlayerSession16setMaxSpectatorsERKi() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V139RequestPlayerSessionMemberPlayerFactory6createEPNS1_6Common10LibContextERKNS5_6VectorINS5_12IntrusivePtrINS3_32RequestCreatePlayerSessionPlayerEEEEEPNS9_INS3_32RequestPlayerSessionMemberPlayerEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEE3endEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi36ParameterToJoinPlayerSessionAsPlayerD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V110FromMember11getOnlineIdEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi38ParameterToGetPlayerSessionInvitations26setinvitationInvalidFilterEPKc(void* self, const char* filter) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SetParameterText(self, filter));
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V152PostPlayerSessionsSessionIdMemberPlayersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V126RequestPlayerSessionPlayerEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V151PostPlayerSessionsSessionIdMemberPlayersRequestBodyEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi29ParameterToLeavePlayerSessionD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V122LocalizedStringFactory6createEPNS1_6Common10LibContextEPKcNS_4Json6ObjectEPNS5_12IntrusivePtrINS3_15LocalizedStringEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V151PostPlayerSessionsSessionIdMemberPlayersRequestBodyEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V132RequestCreatePlayerSessionPlayerEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEED2Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V113PlayerSessionEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V120RequestPlayerSession20setInvitableUserTypeERKNS3_17InvitableUserTypeE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V132RequestCreatePlayerSessionPlayerEEEEC1EPNS2_10LibContextE(IntrusivePtr* self, void* libContext) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    self->object = nullptr;
    self->deleter = nullptr;
    self->libContext = libContext;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V132RequestCreatePlayerSessionPlayerEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_6StringEEC1EPNS2_10LibContextE(IntrusivePtr* self, void* libContext) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    self->object = nullptr;
    self->deleter = nullptr;
    self->libContext = libContext;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEE8pushBackERKS8_(IntrusivePtr* self, const void* element) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(VectorPushBackIntrusivePtr(self, static_cast<const IntrusivePtr*>(element)));
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V152PostPlayerSessionsSessionIdMemberPlayersResponseBodyEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_6StringEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEE5beginEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V129PostPlayerSessionsRequestBodyEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEEEdeEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_15DefaultResponseENS2_12IntrusivePtrINS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V126RequestPlayerSessionPlayerEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V115LocalizedStringEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V151PostPlayerSessionsSessionIdMemberPlayersRequestBodyEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, 0x00);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi25joinPlayerSessionAsPlayerEiRKNS4_36ParameterToJoinPlayerSessionAsPlayerERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_52PostPlayerSessionsSessionIdMemberPlayersResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6String6appendEPKc(CommonString* self, const char* text) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    if (text == nullptr) return netCall.Ret(0);
    const std::size_t textLength = std::strlen(text);
    const std::size_t oldLength = self->buffer != nullptr ? std::strlen(self->buffer) : 0;
    const std::size_t needed = oldLength + textLength + 1;
    if (needed > self->bufferSize) {
        std::size_t capacity = self->bufferSize != 0 ? static_cast<std::size_t>(self->bufferSize) : 16;
        while (capacity < needed) capacity *= 2;
        char* buffer = static_cast<char*>(std::malloc(capacity));
        if (buffer == nullptr) return netCall.Ret(-1);
        if (oldLength != 0) std::memcpy(buffer, self->buffer, oldLength);
        std::free(self->buffer);
        self->buffer = buffer;
        self->bufferSize = capacity;
    }
    std::memcpy(self->buffer + oldLength, text, textLength + 1);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V132RequestCreatePlayerSessionPlayerEEEE8pushBackERKS8_(IntrusivePtr* self, const void* element) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(VectorPushBackIntrusivePtr(self, static_cast<const IntrusivePtr*>(element)));
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15DefaultResponseC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi36ParameterToJoinPlayerSessionAsPlayer9terminateEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TerminateParameter(self));
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi31ParameterToCreatePlayerSessions10initializeEPNS1_6Common10LibContextENS6_12IntrusivePtrINS3_29PostPlayerSessionsRequestBodyEEE(void* self, const std::uint64_t* context, const IntrusivePtr* body) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(InitializeParameterWithBody(self, context, true, body));
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V110FromMemberEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V132RequestPlayerSessionMemberPlayerEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V126RequestPlayerSessionPlayerEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi29ParameterToLeavePlayerSessionC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ConstructParameter(self);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V126RequestPlayerSessionPlayerEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead12getSessionIdEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead7getFromEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V129PostPlayerSessionsRequestBodyEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, 0x00);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V130PostPlayerSessionsResponseBody17getPlayerSessionsEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi18leavePlayerSessionEiRKNS4_29ParameterToLeavePlayerSessionERNS1_6Common11TransactionINS8_15DefaultResponseENS8_12IntrusivePtrINS8_18ResponseHeaderBaseEEEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V124PlayerSessionPushContextEEEE8pushBackERKS8_(IntrusivePtr* self, const void* element) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(VectorPushBackIntrusivePtr(self, static_cast<const IntrusivePtr*>(element)));
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V152PostPlayerSessionsSessionIdMemberPlayersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi31ParameterToCreatePlayerSessionsD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi27getPlayerSessionInvitationsEiRKNS4_38ParameterToGetPlayerSessionInvitationsERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_54GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V132RequestPlayerSessionMemberPlayerEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V113PlayerSessionEEEEEEdeEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V124PlayerSessionPushContextEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V110FromMemberEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi38ParameterToGetPlayerSessionInvitationsC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ConstructParameter(self);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V126RequestPlayerSessionPlayerEEEE8pushBackERKS8_(IntrusivePtr* self, const void* element) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(VectorPushBackIntrusivePtr(self, static_cast<const IntrusivePtr*>(element)));
}

void* APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEppEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V152PostPlayerSessionsSessionIdMemberPlayersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V120RequestPlayerSession19setJoinableUserTypeERKNS3_16JoinableUserTypeE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi38ParameterToGetPlayerSessionInvitationsD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V113PlayerSessionEEEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V132RequestPlayerSessionMemberPlayerEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, 0x00);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead14sessionIdIsSetEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead17invitationIdIsSetEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi31ParameterToCreatePlayerSessionsC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ConstructParameter(self);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi36ParameterToJoinPlayerSessionAsPlayer10initializeEPNS1_6Common10LibContextEPKcNS6_12IntrusivePtrINS3_51PostPlayerSessionsSessionIdMemberPlayersRequestBodyEEE(void* self, const std::uint64_t* context, const char* sessionId, const IntrusivePtr* body) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(InitializeParameterWithBody(self, context, sessionId != nullptr, body));
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBody16invitationsIsSetEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V113PlayerSessionEEEEixEm(void* self, std::uint64_t index) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__, [&](NetBridgeJson& netIn) { netIn.Uint("index", index); });
    return netCall.Ret(HostVectorElement(self, index, __func__));
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi29ParameterToLeavePlayerSession10initializeEPNS1_6Common10LibContextEPKcSA_(void* self, const std::uint64_t* context, const char* sessionId, const char* accountId) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(InitializeParameter(self, context, sessionId != nullptr && accountId != nullptr));
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V115LocalizedStringEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS2_6StringEEEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, 0x30);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEdeEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi38ParameterToGetPlayerSessionInvitations9setfieldsENS1_6Common12IntrusivePtrINS6_6VectorINS6_6StringEEEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15DefaultResponseD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS2_6StringEEEEC1EPS6_PFvS8_EPNS2_10LibContextE(IntrusivePtr* self, void* body, void (APS5_VABI* deleter)(void*), void* libContext) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    self->object = body;
    self->deleter = deleter;
    self->libContext = libContext;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEEC1EPNS2_10LibContextE(IntrusivePtr* self, void* libContext) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    self->object = nullptr;
    self->deleter = nullptr;
    self->libContext = libContext;
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V133RequestPlayerSessionPlayerFactory6createEPNS1_6Common10LibContextEPKcS9_PNS5_12IntrusivePtrINS3_26RequestPlayerSessionPlayerEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi20createPlayerSessionsEiRKNS4_31ParameterToCreatePlayerSessionsERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_30PostPlayerSessionsResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi36ParameterToJoinPlayerSessionAsPlayerC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ConstructParameter(self);
    return netCall.Ret(0);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead9fromIsSetEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V126RequestPlayerSessionPlayer15setPushContextsERKNS1_6Common6VectorINS5_12IntrusivePtrINS3_24PlayerSessionPushContextEEEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V127RequestPlayerSessionFactory6createEPNS1_6Common10LibContextEiNS5_12IntrusivePtrINS3_32RequestPlayerSessionMemberPlayerEEERKNS5_6VectorINS5_6StringEEENS8_INS3_15LocalizedStringEEEPNS8_INS3_20RequestPlayerSessionEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V126RequestPlayerSessionPlayerEEEEC1EPNS2_10LibContextE(IntrusivePtr* self, void* libContext) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    self->object = nullptr;
    self->deleter = nullptr;
    self->libContext = libContext;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V124PlayerSessionPushContextEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V131PlayerSessionPushContextFactory6createEPNS1_6Common10LibContextEPKcPNS5_12IntrusivePtrINS3_24PlayerSessionPushContextEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V129PostPlayerSessionsRequestBodyEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_6StringEE8pushBackERKS4_(IntrusivePtr* self, const void* element) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(VectorPushBackString(self, static_cast<const CommonString*>(element)));
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEEEneERKS9_(const void* self, const void* other) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    (void)other;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi38ParameterToGetPlayerSessionInvitations10initializeEPNS1_6Common10LibContextEPKc(void* self, const std::uint64_t* context, const char* accountId) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(InitializeParameter(self, context, accountId != nullptr));
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS2_6StringEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBody14getInvitationsEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V132RequestCreatePlayerSessionPlayerEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V152PostPlayerSessionsSessionIdMemberPlayersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_15DefaultResponseENS2_12IntrusivePtrINS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_15DefaultResponseENS2_12IntrusivePtrINS2_18ResponseHeaderBaseEEEE6finishEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V136UsersPlayerSessionsInvitationForRead22receivedTimestampIsSetEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120RequestPlayerSessionEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_15DefaultResponseENS2_12IntrusivePtrINS2_18ResponseHeaderBaseEEEE11getResponseERS4_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V154GetUsersAccountIdPlayerSessionsInvitationsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V139RequestCreatePlayerSessionPlayerFactory6createEPNS1_6Common10LibContextEPKcS9_RKNS5_6VectorINS5_12IntrusivePtrINS3_24PlayerSessionPushContextEEEEEPNSB_INS3_32RequestCreatePlayerSessionPlayerEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V152PostPlayerSessionsSessionIdMemberPlayersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V124PlayerSessionPushContextEEEEC1EPNS2_10LibContextE(IntrusivePtr* self, void* libContext) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    self->object = nullptr;
    self->deleter = nullptr;
    self->libContext = libContext;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V124PlayerSessionPushContextEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V136UsersPlayerSessionsInvitationForReadEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V152PostPlayerSessionsSessionIdMemberPlayersResponseBodyEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V113PlayerSession12getSessionIdEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V158PostPlayerSessionsSessionIdMemberPlayersRequestBodyFactory6createEPNS1_6Common10LibContextERKNS5_6VectorINS5_12IntrusivePtrINS3_26RequestPlayerSessionPlayerEEEEEPNS9_INS3_51PostPlayerSessionsSessionIdMemberPlayersRequestBodyEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V136PostPlayerSessionsRequestBodyFactory6createEPNS1_6Common10LibContextERKNS5_6VectorINS5_12IntrusivePtrINS3_20RequestPlayerSessionEEEEEPNS9_INS3_29PostPlayerSessionsRequestBodyEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V120RequestPlayerSession28setExclusiveLeaderPrivilegesERKNS1_6Common6VectorINS5_6StringEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_15DefaultResponseENS2_12IntrusivePtrINS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_12IntrusivePtrINS1_13InGameCatalog2V57ProductEEEED2Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEED2Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V59ContainerEEEEEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V59ContainerEEEEEEdeEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V59ContainerEEEEEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V59ContainerEEEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V57ProductEEEEEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V57ProductEEEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V53SkuEEEEEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V53SkuEEEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_17TitleCloudStorage2V110DataStatusEEEEEEdeEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_17TitleCloudStorage2V110DataStatusEEEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_13InGameCatalog2V59ContainerEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, 0x00);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_13InGameCatalog2V59ContainerEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_13InGameCatalog2V59ContainerEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_13InGameCatalog2V57ProductEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, 0x00);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_13InGameCatalog2V57ProductEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_13InGameCatalog2V57ProductEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V15OwnerEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_17TitleCloudStorage2V15OwnerEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_13InGameCatalog2V57ProductEEEEneERKS9_(const void* self, const void* other) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    (void)other;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_13InGameCatalog2V57ProductEEEEdeEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_13InGameCatalog2V57ProductEEEEppEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEEneERKS9_(const void* self, const void* other) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    (void)other;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEEdeEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEEppEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS2_6VectorINS4_INS1_13InGameCatalog2V59ContainerEEEEEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    std::memset(self, 0, 0x10);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS2_6VectorINS4_INS1_13InGameCatalog2V59ContainerEEEEEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERSB_(const void* self, void* response) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    (void)response;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS2_6VectorINS4_INS1_13InGameCatalog2V59ContainerEEEEEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    std::memset(self, 0, 0x10);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_(const void* self, void* response) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    (void)response;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS2_6VectorINS4_INS1_13InGameCatalog2V59ContainerEEEEEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS2_6VectorINS4_INS1_13InGameCatalog2V59ContainerEEEEEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE(void* self, void* context) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    (void)context;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_17TitleCloudStorage2V132GetMultiDataStatusesResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE(void* self, void* context) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    (void)context;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_13InGameCatalog2V59ContainerEEEEixEm(void* self, std::uint64_t index) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__, [&](NetBridgeJson& netIn) { netIn.Uint("index", index); });
    return netCall.Ret(HostVectorElement(self, index, __func__));
}

std::size_t APS5_VABI _ZNK3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_13InGameCatalog2V59ContainerEEEE4sizeEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(HostVectorSize(self, __func__));
}

void* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_13InGameCatalog2V57ProductEEEE5beginEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_13InGameCatalog2V57ProductEEEE3endEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

std::size_t APS5_VABI _ZNK3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_13InGameCatalog2V53SkuEEEE4sizeEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(HostVectorSize(self, __func__));
}

void* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEE5beginEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_17TitleCloudStorage2V110DataStatusEEEE3endEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V59Container11getChildrenEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

int APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi23ParameterToGetContainerC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ConstructParameter(self);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi23ParameterToGetContainer10initializeEPNS1_6Common10LibContextE(void* self, const std::uint64_t* context) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(InitializeParameter(self, context, true));
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi23ParameterToGetContainer13isInitializedEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(static_cast<const ParameterObject*>(self)->implementation != nullptr);
}

int APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi23ParameterToGetContainer15setserviceLabelEi(void* self, int serviceLabel) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    (void)serviceLabel;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi23ParameterToGetContainer9terminateEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TerminateParameter(self));
}

int APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi23ParameterToGetContainerD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi12getContainerEiRKNS4_23ParameterToGetContainerERNS1_6Common11TransactionINS8_12IntrusivePtrINS8_6VectorINSA_INS3_9ContainerEEEEEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V57Product8getLabelEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V57Product7getSkusEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V57Product7getTypeEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

bool APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V57Product9skusIsSetEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(false);
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi23ParameterToDownloadData11setobjectIdEPKc(void* self, const char* objectId) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SetParameterText(self, objectId));
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUserC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ConstructParameter(self);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUser10initializeEPNS1_6Common10LibContextEPKcSA_(void* self, const std::uint64_t* context, const char* first, const char* second) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(InitializeParameter(self, context, first != nullptr && second != nullptr));
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUser9setfieldsEPKc(void* self, const char* fields) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SetParameterText(self, fields));
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUser9terminateEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TerminateParameter(self));
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi37ParameterToGetMultiDataStatusesByUserD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V17DataApi26getMultiDataStatusesByUserEiRKNS4_37ParameterToGetMultiDataStatusesByUserERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_32GetMultiDataStatusesResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE(void) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

std::size_t APS5_VABI _ZNK3sce2Np9CppWebApi17TitleCloudStorage2V110DataStatus11getDataSizeEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

const char* APS5_VABI _ZNK3sce2Np9CppWebApi17TitleCloudStorage2V110DataStatus11getObjectIdEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi17TitleCloudStorage2V110DataStatus8getOwnerEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi17TitleCloudStorage2V110DataStatus9getSlotIdEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi17TitleCloudStorage2V132GetMultiDataStatusesResponseBody17getDataStatusListEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(nullptr);
}

std::uint32_t APS5_VABI _ZNK3sce2Np9CppWebApi17TitleCloudStorage2V132GetMultiDataStatusesResponseBody23getTotalDataStatusCountEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

std::uint64_t APS5_VABI _ZNK3sce2Np9CppWebApi17TitleCloudStorage2V15Owner12getAccountIdEv(const void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V110FriendsApi10getFriendsEiRKNS4_21ParameterToGetFriendsERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_18GetFriendsResponseEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V110FriendsApi21ParameterToGetFriends10initializeEPNS1_6Common10LibContextEPKc() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V110FriendsApi21ParameterToGetFriends9terminateEv() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V110FriendsApi21ParameterToGetFriendsC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V110FriendsApi21ParameterToGetFriendsD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V111PresenceApi17getBasicPresencesEiRKNS4_28ParameterToGetBasicPresencesERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_25GetBasicPresencesResponseEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V111PresenceApi28ParameterToGetBasicPresences10initializeEPNS1_6Common10LibContextEPKc() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V111PresenceApi28ParameterToGetBasicPresences9terminateEv() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V111PresenceApi28ParameterToGetBasicPresencesC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V111PresenceApi28ParameterToGetBasicPresencesD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V118GetFriendsResponse10getFriendsEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

// --------------------------------------------------------------------------
// SessionManager2V1 PlayerSessionsApi + InGameCatalog2V5 ContainerApi and Common
// container helpers imported by titles whose dumps lack libSceNpCppWebApi.prx:
// request entry points and transactions fail SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN
// offline; parameter bookkeeping succeeds; response accessors stay unimplemented.
// --------------------------------------------------------------------------

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V149PostPlayerSessionsSessionIdInvitationsRequestBody14setInvitationsERKNS1_6Common6VectorINS5_12IntrusivePtrINS3_30RequestPlayerSessionInvitationEEEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi28sendPlayerSessionInvitationsEiRKNS4_39ParameterToSendPlayerSessionInvitationsERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_50PostPlayerSessionsSessionIdInvitationsResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi17getPlayerSessionsEiRKNS4_28ParameterToGetPlayerSessionsERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_29GetPlayerSessionsResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V120RequestPlayerSession14setCustomData2EPKvm() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi49ParameterToAddPlayerSessionJoinableSpecifiedUsers10initializeEPNS1_6Common10LibContextEPKcNS6_12IntrusivePtrINS3_60PostPlayerSessionsSessionIdJoinableSpecifiedUsersRequestBodyEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi37ParameterToSetPlayerSessionProperties10initializeEPNS1_6Common10LibContextEPKcNS6_12IntrusivePtrINS3_39PatchPlayerSessionsSessionIdRequestBodyEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi36ParameterToChangePlayerSessionLeader9terminateEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi37ParameterToSetPlayerSessionPropertiesC1Ev() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi49ParameterToAddPlayerSessionJoinableSpecifiedUsers9terminateEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V120RequestPlayerSession14setCustomData1EPKvm() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V120PlayerSessionForRead16customData2IsSetEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V124GetBlockingUsersResponse9getBlocksEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi37ParameterToSetPlayerSessionPropertiesD1Ev() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi39ParameterToSendPlayerSessionInvitations10initializeEPNS1_6Common10LibContextEPKcNS6_12IntrusivePtrINS3_49PostPlayerSessionsSessionIdInvitationsRequestBodyEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V146PatchPlayerSessionsSessionIdRequestBodyFactory6createEPNS1_6Common10LibContextEPNS5_12IntrusivePtrINS3_39PatchPlayerSessionsSessionIdRequestBodyEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V137RequestPlayerSessionInvitationFactory6createEPNS1_6Common10LibContextENS5_12IntrusivePtrINS3_2ToEEEPNS8_INS3_30RequestPlayerSessionInvitationEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V139PatchPlayerSessionsSessionIdRequestBody19setJoinableUserTypeERKNS3_22CustomJoinableUserTypeE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V127ResponsePlayerSessionPlayer11getPlatformEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V125GetBasicPresencesResponse17getBasicPresencesEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi28ParameterToGetPlayerSessions9setfieldsENS1_6Common12IntrusivePtrINS6_6VectorINS6_6StringEEEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi28ParameterToGetPlayerSessions9terminateEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi29ParameterToLeavePlayerSession9terminateEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TerminateParameter(self));
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi39ParameterToSendPlayerSessionInvitationsD1Ev() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V120PlayerSessionForRead14getCustomData1Ev() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V19BlocksApi16getBlockingUsersEiRKNS4_27ParameterToGetBlockingUsersERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_24GetBlockingUsersResponseEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V19BlocksApi27ParameterToGetBlockingUsers10initializeEPNS1_6Common10LibContextE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V19BlocksApi27ParameterToGetBlockingUsers9terminateEv() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V19BlocksApi27ParameterToGetBlockingUsersC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi31ParameterToCreatePlayerSessions9terminateEv(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TerminateParameter(self));
}

int APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V120PlayerSessionForRead16customData1IsSetEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi38addPlayerSessionJoinableSpecifiedUsersEiRKNS4_49ParameterToAddPlayerSessionJoinableSpecifiedUsersERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_61PostPlayerSessionsSessionIdJoinableSpecifiedUsersResponseBodyEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V120PlayerSessionForRead9getLeaderEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi39ParameterToSendPlayerSessionInvitationsC1Ev() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi28ParameterToGetPlayerSessionsD1Ev() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V120PlayerSessionForRead28getExclusiveLeaderPrivilegesEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi26setPlayerSessionPropertiesEiRKNS4_37ParameterToSetPlayerSessionPropertiesERNS1_6Common11TransactionINS8_15DefaultResponseENS8_12IntrusivePtrINS8_18ResponseHeaderBaseEEEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi25changePlayerSessionLeaderEiRKNS4_36ParameterToChangePlayerSessionLeaderERNS1_6Common11TransactionINS8_15DefaultResponseENS8_12IntrusivePtrINS8_18ResponseHeaderBaseEEEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V150PutPlayerSessionsSessionIdLeaderRequestBodyFactory6createEPNS1_6Common10LibContextERKNS_4Json5ValueEPNS5_12IntrusivePtrINS3_43PutPlayerSessionsSessionIdLeaderRequestBodyEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V119JoinableUserFactory6createEPNS1_6Common10LibContextERKmPNS5_12IntrusivePtrINS3_12JoinableUserEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi36ParameterToChangePlayerSessionLeaderD1Ev() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V156PostPlayerSessionsSessionIdInvitationsRequestBodyFactory6createEPNS1_6Common10LibContextEPNS5_12IntrusivePtrINS3_49PostPlayerSessionsSessionIdInvitationsRequestBodyEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi39ParameterToSendPlayerSessionInvitations9terminateEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi28ParameterToGetPlayerSessions10initializeEPNS1_6Common10LibContextEPKc() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi28ParameterToGetPlayerSessionsC1Ev() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V133ResponsePlayerSessionMemberPlayer10getPlayersEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V139PatchPlayerSessionsSessionIdRequestBody28setExclusiveLeaderPrivilegesERKNS1_6Common6VectorINS5_6StringEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V113PlayerSession9getMemberEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V19ToFactory6createEPNS1_6Common10LibContextERKmPNS5_12IntrusivePtrINS3_2ToEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi49ParameterToAddPlayerSessionJoinableSpecifiedUsersD1Ev() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi36ParameterToChangePlayerSessionLeader10initializeEPNS1_6Common10LibContextEPKcNS6_12IntrusivePtrINS3_43PutPlayerSessionsSessionIdLeaderRequestBodyEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V167PostPlayerSessionsSessionIdJoinableSpecifiedUsersRequestBodyFactory6createEPNS1_6Common10LibContextERKNS5_6VectorINS5_12IntrusivePtrINS3_12JoinableUserEEEEEPNS9_INS3_60PostPlayerSessionsSessionIdJoinableSpecifiedUsersRequestBodyEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi36ParameterToChangePlayerSessionLeaderC1Ev() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi37ParameterToSetPlayerSessionProperties9terminateEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V118LeaderWithOnlineId12getAccountIdEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V139PatchPlayerSessionsSessionIdRequestBody20setInvitableUserTypeERKNS3_23CustomInvitableUserTypeE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V129GetPlayerSessionsResponseBody17getPlayerSessionsEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi14SessionManager2V117PlayerSessionsApi49ParameterToAddPlayerSessionJoinableSpecifiedUsersC1Ev() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V120PlayerSessionForRead14getCustomData2Ev() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi14SessionManager2V127ResponsePlayerSessionPlayer12getAccountIdEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11hasResponseEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V112JoinableUserEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V129GetPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V19BlocksApi27ParameterToGetBlockingUsersD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi15ProfanityFilter2V212ProfanityApi15filterProfanityEiRKNS4_26ParameterToFilterProfanityERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_23FilterProfanityResponseEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi15ProfanityFilter2V212ProfanityApi26ParameterToFilterProfanity10initializeEPNS1_6Common10LibContextEPKcSA_NS6_12IntrusivePtrINS3_19WebApiFilterRequestEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi15ProfanityFilter2V212ProfanityApi26ParameterToFilterProfanity9terminateEv() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi15ProfanityFilter2V212ProfanityApi26ParameterToFilterProfanityC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi15ProfanityFilter2V212ProfanityApi26ParameterToFilterProfanityD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi15ProfanityFilter2V226WebApiFilterRequestFactory6createEPNS1_6Common10LibContextEPKcPNS5_12IntrusivePtrINS3_19WebApiFilterRequestEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi30CommunicationRestrictionStatus2V333CommunicationRestrictionStatusApi33getCommunicationRestrictionStatusEiRKNS4_44ParameterToGetCommunicationRestrictionStatusERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_38CommunicationRestrictionStatusResponseEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi30CommunicationRestrictionStatus2V333CommunicationRestrictionStatusApi44ParameterToGetCommunicationRestrictionStatus10initializeEPNS1_6Common10LibContextEm() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi30CommunicationRestrictionStatus2V333CommunicationRestrictionStatusApi44ParameterToGetCommunicationRestrictionStatus9terminateEv() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi30CommunicationRestrictionStatus2V333CommunicationRestrictionStatusApi44ParameterToGetCommunicationRestrictionStatusC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi30CommunicationRestrictionStatus2V333CommunicationRestrictionStatusApi44ParameterToGetCommunicationRestrictionStatusD1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEE21setOnFinishedCallbackEPFvliS8_PvEPFvliSC_ESC_() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEE21setOnFinishedCallbackEPFvliS8_PvEPFvliSC_ESC_() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE21setOnFinishedCallbackEPFvliS8_PvEPFvliSC_ESC_() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_15ProfanityFilter2V223FilterProfanityResponseEEENS4_INS2_18ResponseHeaderBaseEEEE21setOnFinishedCallbackEPFvliS8_PvEPFvliSC_ESC_() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_15ProfanityFilter2V223FilterProfanityResponseEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_15ProfanityFilter2V223FilterProfanityResponseEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEE21setOnFinishedCallbackEPFvliS8_PvEPFvliSC_ESC_() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_15ProfanityFilter2V219WebApiFilterRequestEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, ReferenceCountOffset);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_15ProfanityFilter2V219WebApiFilterRequestEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_15ProfanityFilter2V219WebApiFilterRequestEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, ReferenceCountOffset, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_18ResponseHeaderBaseEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_18ResponseHeaderBaseEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, ReferenceCountOffset, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6StringEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6StringEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, ReferenceCountOffset, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_11UserProfile2V113BasicPresenceEEEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, VectorReferenceCountOffset, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEEEED2Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V112JoinableUserEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_14SessionManager2V120PlayerSessionForReadEEEEdeEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V118LeaderWithOnlineIdEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V112JoinableUserEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V129GetPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11hasResponseEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V160PostPlayerSessionsSessionIdJoinableSpecifiedUsersRequestBodyEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V143PutPlayerSessionsSessionIdLeaderRequestBodyEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V129GetPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorImED2Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V161PostPlayerSessionsSessionIdJoinableSpecifiedUsersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V127ResponsePlayerSessionPlayerEEEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V129GetPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V129GetPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5abortEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V149PostPlayerSessionsSessionIdInvitationsRequestBodyEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V129GetPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V112JoinableUserEEEE8pushBackERKS8_(IntrusivePtr* self, const void* element) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(VectorPushBackIntrusivePtr(self, static_cast<const IntrusivePtr*>(element)));
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V149PostPlayerSessionsSessionIdInvitationsRequestBodyEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, 0x00);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V150PostPlayerSessionsSessionIdInvitationsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V113PlayerSessionEEaSERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    AssignIntrusivePtr(self, source, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V12ToEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V129GetPlayerSessionsResponseBodyEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V160PostPlayerSessionsSessionIdJoinableSpecifiedUsersRequestBodyEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, 0x00);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V120PlayerSessionForReadEEEEEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V161PostPlayerSessionsSessionIdJoinableSpecifiedUsersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEE28setResponseInformationOptionEP37SceNpWebApi2ResponseInformationOption() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEE28setResponseInformationOptionEP37SceNpWebApi2ResponseInformationOption() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE28setResponseInformationOptionEP37SceNpWebApi2ResponseInformationOption() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_15ProfanityFilter2V223FilterProfanityResponseEEENS4_INS2_18ResponseHeaderBaseEEEE28setResponseInformationOptionEP37SceNpWebApi2ResponseInformationOption() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_15ProfanityFilter2V223FilterProfanityResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_15ProfanityFilter2V223FilterProfanityResponseEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEE28setResponseInformationOptionEP37SceNpWebApi2ResponseInformationOption() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common18ResponseHeaderBase14getHeaderValueEPKcRNS2_12IntrusivePtrINS2_6StringEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V150PostPlayerSessionsSessionIdInvitationsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5abortEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEEEE3endEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEEEE5beginEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorImE3endEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorImE5beginEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEEEEppEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorImEppEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi11UserProfile2V113BasicPresence12getAccountIdEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi11UserProfile2V113BasicPresence12getInContextEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi11UserProfile2V113BasicPresence15getOnlineStatusEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi15ProfanityFilter2V223FilterProfanityResponse10getMessageEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponse13getRestrictedEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_15ProfanityFilter2V223FilterProfanityResponseEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_18ResponseHeaderBaseEE3getEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_18ResponseHeaderBaseEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6StringEE3getEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6StringEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_11UserProfile2V113BasicPresenceEEEEEEdeEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_11UserProfile2V113BasicPresenceEEEEEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorImEEEdeEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorImEEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEE18getResponseHeadersERSA_() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEE18getResponseHeadersERSA_() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE18getResponseHeadersERSA_() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_15ProfanityFilter2V223FilterProfanityResponseEEENS4_INS2_18ResponseHeaderBaseEEEE18getResponseHeadersERSA_() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEE18getResponseHeadersERSA_() {
 NetBridgeCall netCall("libSceNpCppWebApi", __func__);
 return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEEEE4sizeEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V129GetPlayerSessionsResponseBodyEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120PlayerSessionForReadEEEE5emptyEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    const auto* body = static_cast<const VectorBody*>(self->object);
    if (body == nullptr) return netCall.Ret(1);
    if (body->magic != VectorBodyMagic) {
        NotImplemented_nid_no_patch(__func__);
        return netCall.Ret(0);
    }
    return netCall.Ret(body->size == 0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V150PostPlayerSessionsSessionIdInvitationsResponseBodyEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V143PutPlayerSessionsSessionIdLeaderRequestBodyEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, 0x00);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V139PatchPlayerSessionsSessionIdRequestBodyEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, 0x00);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V127ResponsePlayerSessionPlayerEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V150PostPlayerSessionsSessionIdInvitationsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V150PostPlayerSessionsSessionIdInvitationsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V139PatchPlayerSessionsSessionIdRequestBodyEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V113PlayerSessionEEEEEEaSERKSA_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    AssignIntrusivePtr(self, source, 0x30, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V12ToEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, 0x00);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V113PlayerSessionEEEEEE3getEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V149PostPlayerSessionsSessionIdInvitationsRequestBodyEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V127ResponsePlayerSessionPlayerEEEEEE3getEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V112JoinableUserEEEEC1EPNS2_10LibContextE(IntrusivePtr* self, void* libContext) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    self->object = nullptr;
    self->deleter = nullptr;
    self->libContext = libContext;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V130RequestPlayerSessionInvitationEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V127ResponsePlayerSessionPlayerEEEEC1EPNS2_10LibContextE(IntrusivePtr* self, void* libContext) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    self->object = nullptr;
    self->deleter = nullptr;
    self->libContext = libContext;
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V129GetPlayerSessionsResponseBodyEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V160PostPlayerSessionsSessionIdJoinableSpecifiedUsersRequestBodyEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V150PostPlayerSessionsSessionIdInvitationsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V12ToEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V161PostPlayerSessionsSessionIdJoinableSpecifiedUsersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5abortEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common6VectorImE4sizeEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V129GetPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5getIdEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEEEEdeEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V152PostPlayerSessionsSessionIdMemberPlayersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11hasResponseEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

void* APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V127ResponsePlayerSessionPlayerEEEEixEm(IntrusivePtr* self, unsigned long index) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__, [&](NetBridgeJson& netIn) { netIn.Uint("index", index); });
    auto* body = static_cast<VectorBody*>(self->object);
    if (body == nullptr || body->magic != VectorBodyMagic || body->elementKind != 1) {
        NotImplemented_nid_no_patch(__func__);
        return netCall.Ret(nullptr);
    }
    return netCall.Ret(static_cast<IntrusivePtr*>(body->elements) + index);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V127ResponsePlayerSessionPlayerEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_12IntrusivePtrINS1_14SessionManager2V120PlayerSessionForReadEEEED2Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V150PostPlayerSessionsSessionIdInvitationsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V143PutPlayerSessionsSessionIdLeaderRequestBodyEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V161PostPlayerSessionsSessionIdJoinableSpecifiedUsersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5abortEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEEEEneERKS9_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V139PatchPlayerSessionsSessionIdRequestBodyEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V113PlayerSessionEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V139PatchPlayerSessionsSessionIdRequestBodyEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V113PlayerSessionEE3getEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V161PostPlayerSessionsSessionIdJoinableSpecifiedUsersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11hasResponseEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V150PostPlayerSessionsSessionIdInvitationsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11hasResponseEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V133ResponsePlayerSessionMemberPlayerEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V130RequestPlayerSessionInvitationEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V133ResponsePlayerSessionMemberPlayerEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_14SessionManager2V129GetPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V113PlayerSessionEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V130RequestPlayerSessionInvitationEEEEC1EPNS2_10LibContextE(IntrusivePtr* self, void* libContext) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    self->object = nullptr;
    self->deleter = nullptr;
    self->libContext = libContext;
}

std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V161PostPlayerSessionsSessionIdJoinableSpecifiedUsersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5getIdEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorImEdeEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V118LeaderWithOnlineIdEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V130PostPlayerSessionsResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5getIdEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorImEneERKS4_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

std::uint64_t APS5_VABI _ZNK3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V127ResponsePlayerSessionPlayerEEEE4sizeEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    const auto* body = static_cast<const VectorBody*>(self->object);
    if (body == nullptr) return netCall.Ret(0);
    if (body->magic != VectorBodyMagic) {
        NotImplemented_nid_no_patch(__func__);
        return netCall.Ret(0);
    }
    return netCall.Ret(body->size);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V120PlayerSessionForReadEEEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V120PlayerSessionForReadEEEE5beginEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V130RequestPlayerSessionInvitationEEEE8pushBackERKS8_(IntrusivePtr* self, const void* element) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(VectorPushBackIntrusivePtr(self, static_cast<const IntrusivePtr*>(element)));
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V161PostPlayerSessionsSessionIdJoinableSpecifiedUsersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V152PostPlayerSessionsSessionIdMemberPlayersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5abortEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V149PostPlayerSessionsSessionIdInvitationsRequestBodyEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V130RequestPlayerSessionInvitationEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V120PlayerSessionForReadEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_14SessionManager2V152PostPlayerSessionsSessionIdMemberPlayersResponseBodyEEENS4_INS2_18ResponseHeaderBaseEEEE5getIdEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_14SessionManager2V127ResponsePlayerSessionPlayerEEEE8copyFromERKS9_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_14SessionManager2V150PostPlayerSessionsSessionIdInvitationsResponseBodyEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_14SessionManager2V113PlayerSessionEEEEEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

int APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V53Sku10labelIsSetEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V53Sku10priceIsSetEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V53Sku8getPriceEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V53Sku15getDisplayPriceEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi23ParameterToGetContainer7setsortENS5_4SortE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V53Sku18originalPriceIsSetEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V57Product14getDescriptionEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V57Product14getReleaseDateEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V53Sku23getDisplayOriginalPriceEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V57Product14getDisplayNameEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V53Sku16getOriginalPriceEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V53Sku25displayOriginalPriceIsSetEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V53Sku8getLabelEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi23ParameterToGetContainer9setoffsetEi() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V57Product16releaseDateIsSetEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi23ParameterToGetContainer15setcontainerIdsEPKc(void* self, const char* containerIds) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SetParameterText(self, containerIds));
}

int APS5_VABI _ZN3sce2Np9CppWebApi13InGameCatalog2V512ContainerApi23ParameterToGetContainer8setlimitEi() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V53Sku17displayPriceIsSetEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi13InGameCatalog2V57Product5getIdEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS2_6VectorINS4_INS1_13InGameCatalog2V59ContainerEEEEEEENS4_INS2_18ResponseHeaderBaseEEEE11hasResponseEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS2_6VectorINS4_INS1_13InGameCatalog2V59ContainerEEEEEEENS4_INS2_18ResponseHeaderBaseEEEEaSERKSE_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_13InGameCatalog2V59ContainerEEEEppEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_13InGameCatalog2V57ProductEEEE5emptyEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    const auto* body = static_cast<const VectorBody*>(self->object);
    if (body == nullptr) return netCall.Ret(1);
    if (body->magic != VectorBodyMagic) {
        NotImplemented_nid_no_patch(__func__);
        return netCall.Ret(0);
    }
    return netCall.Ret(body->size == 0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_13InGameCatalog2V59ContainerEEEEneERKS9_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_13InGameCatalog2V53SkuEEEE5beginEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_13InGameCatalog2V53SkuEEEEneERKS9_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V57ProductEEEEEEdeEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_13InGameCatalog2V53SkuEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_12IntrusivePtrINS1_13InGameCatalog2V53SkuEEEED2Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_13InGameCatalog2V59ContainerEEEE3endEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_13InGameCatalog2V59ContainerEEEEdeEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_13InGameCatalog2V53SkuEEEEdeEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V59ContainerEEEEEEC1ERKSA_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, 0x30);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS2_6VectorINS4_INS1_13InGameCatalog2V59ContainerEEEEEEENS4_INS2_18ResponseHeaderBaseEEEEC1ERKSE_(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_12IntrusivePtrINS1_13InGameCatalog2V59ContainerEEEED2Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_13InGameCatalog2V59ContainerEEEE5beginEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_13InGameCatalog2V53SkuEEEE3endEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V53SkuEEEEEEdeEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_13InGameCatalog2V53SkuEEEEppEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_13InGameCatalog2V59ContainerEEEE5emptyEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    const auto* body = static_cast<const VectorBody*>(self->object);
    if (body == nullptr) return netCall.Ret(1);
    if (body->magic != VectorBodyMagic) {
        NotImplemented_nid_no_patch(__func__);
        return netCall.Ret(0);
    }
    return netCall.Ret(body->size == 0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_13InGameCatalog2V59ContainerEEEEEEaSERKSA_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    AssignIntrusivePtr(self, source, 0x30, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorINS2_6StringEEppEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorImEEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorImEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_6StringEED2Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_6StringEE8copyFromERKS5_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorImEEE3getEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_6StringEEneERKS5_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_15DefaultResponseENS2_12IntrusivePtrINS2_18ResponseHeaderBaseEEEE11hasResponseEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS2_6StringEEEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_6StringEE3endEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6StringeqEPKc(const CommonString* self, const char* other) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(other != nullptr && std::strcmp(self->buffer != nullptr ? self->buffer : "", other) == 0);
}

std::int64_t APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_15DefaultResponseENS2_12IntrusivePtrINS2_18ResponseHeaderBaseEEEE5getIdEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorImEEEaSERS6_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
    CopyIntrusivePtr(self, source, 0x30);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_15DefaultResponseENS2_12IntrusivePtrINS2_18ResponseHeaderBaseEEEE5abortEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(TransactionNotStarted);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_6StringEE5beginEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common6String5emptyEv(const CommonString* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->buffer == nullptr || self->buffer[0] == '\0');
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS2_6StringEEEE3getEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_6StringEEdeEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

// --------------------------------------------------------------------------
// UserProfile2V1 model accessors, CommunicationRestrictionStatus plumbing and
// the remaining Common instantiations, from the same dump whose modules lack
// libSceNpCppWebApi.prx.
// --------------------------------------------------------------------------

int APS5_VABI _ZNK3sce2Np9CppWebApi11UserProfile2V112BasicProfile19personalDetailIsSetEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V115BasicProfileApi28ParameterToGetPublicProfiles9terminateEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V112BasicProfileC1EPNS1_6Common10LibContextE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEE11hasResponseEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V115BasicProfileApi28ParameterToGetPublicProfiles10initializeEPNS1_6Common10LibContextEPKc() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V125GetPublicProfilesResponseEEENS4_INS2_18ResponseHeaderBaseEEEED1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi11UserProfile2V112BasicProfile25isOfficiallyVerifiedIsSetEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi11UserProfile2V113BasicPresence14accountIdIsSetEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_11UserProfile2V112BasicProfileEEEEEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V113BasicPresence12setAccountIdERKm() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V113BasicPresenceC1EPNS1_6Common10LibContextE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5abortEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V114PersonalDetailEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5getIdEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEE11hasResponseEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V113BasicPresence12setInContextERKb() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_11UserProfile2V112BasicProfileEEEEEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEE11hasResponseEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V110FriendsApi21ParameterToGetFriends9setoffsetEi() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V125GetPublicProfilesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE6finishEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V112BasicProfile11setOnlineIdERK13SceNpOnlineId() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V125GetPublicProfilesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_11UserProfile2V113BasicPresenceEEEEEE3getEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi11UserProfile2V112BasicProfile17getPersonalDetailEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEEC1ERKSB_(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_11UserProfile2V112BasicProfileEEEEEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi11UserProfile2V112BasicProfile11getOnlineIdEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_11UserProfile2V113BasicPresenceEEEEEEaSERKSA_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
    CopyIntrusivePtr(self, source, 0x30);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_11UserProfile2V112BasicProfileEEEE5beginEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi11UserProfile2V114PersonalDetail14getDisplayNameEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi11UserProfile2V112BasicProfile13onlineIdIsSetEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi11UserProfile2V113BasicPresence14inContextIsSetEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V125GetPublicProfilesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE11hasResponseEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V114PersonalDetailEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEC1ERKS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, 0x00);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V112BasicProfileEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V112BasicProfileD1Ev() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_11UserProfile2V112BasicProfileEEEEneERKS9_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V113BasicPresenceEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, 0x00);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V125GetPublicProfilesResponseEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

int APS5_VABI _ZNK3sce2Np9CppWebApi11UserProfile2V112BasicProfile23getIsOfficiallyVerifiedEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_11UserProfile2V112BasicProfileEEEEEE3getEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V115BasicProfileApi17getPublicProfilesEiRKNS4_28ParameterToGetPublicProfilesERNS1_6Common11TransactionINS8_12IntrusivePtrINS3_25GetPublicProfilesResponseEEENSA_INS8_18ResponseHeaderBaseEEEEE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_11UserProfile2V113BasicPresenceEEEEEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS2_6VectorINS3_INS1_11UserProfile2V112BasicProfileEEEEEEaSERKSA_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x30, __func__);
    CopyIntrusivePtr(self, source, 0x30);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE11hasResponseEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V125GetPublicProfilesResponseEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V125GetPublicProfilesResponseEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V125GetPublicProfilesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5startEPNS2_10LibContextE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V118GetFriendsResponseEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V112BasicProfileEEC1ERS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    CopyIntrusivePtr(self, source, 0x00);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V125GetPublicProfilesResponse11getProfilesEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5getIdEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_11UserProfile2V112BasicProfileEEEEppEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V110FriendsApi21ParameterToGetFriends8setlimitEi() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5abortEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEC1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    *self = {};
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V112BasicProfile23setIsOfficiallyVerifiedERKb() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi11UserProfile2V113BasicPresence17onlineStatusIsSetEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V115BasicProfileApi28ParameterToGetPublicProfilesD1Ev() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V115BasicProfileApi28ParameterToGetPublicProfilesC1Ev() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEaSERKS7_(IntrusivePtr* self, const IntrusivePtr* source) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
    CopyIntrusivePtr(self, source, 0x00);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5getIdEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V113BasicPresenceD1Ev() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V125GetPublicProfilesResponseEEENS4_INS2_18ResponseHeaderBaseEEEEC1Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

void APS5_VABI _ZN3sce2Np9CppWebApi6Common12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEED1Ev(IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    ReleaseIntrusivePtr(self, 0x00, __func__);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common13ConstIteratorINS2_12IntrusivePtrINS1_11UserProfile2V112BasicProfileEEEED2Ev(void* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    (void)self;
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V125GetBasicPresencesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5abortEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V19BlocksApi27ParameterToGetBlockingUsers8setlimitEi() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V125GetPublicProfilesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5getIdEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

void* APS5_VABI _ZNK3sce2Np9CppWebApi6Common12IntrusivePtrINS1_11UserProfile2V112BasicProfileEEptEv(const IntrusivePtr* self) {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(self->object);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V19BlocksApi27ParameterToGetBlockingUsers9setoffsetEi() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common6VectorINS2_12IntrusivePtrINS1_11UserProfile2V112BasicProfileEEEE3endEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi11UserProfile2V113BasicPresence15setOnlineStatusERKNS3_12OnlineStatusE() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_11UserProfile2V125GetPublicProfilesResponseEEENS4_INS2_18ResponseHeaderBaseEEEE5abortEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common8IteratorINS2_12IntrusivePtrINS1_11UserProfile2V112BasicProfileEEEEdeEv() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZN3sce2Np9CppWebApi6Common15TransactionBaseINS2_12IntrusivePtrINS1_30CommunicationRestrictionStatus2V338CommunicationRestrictionStatusResponseEEENS4_INS2_18ResponseHeaderBaseEEEEaSERKSB_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    NotImplemented_nid_no_patch(__func__);
    return netCall.Ret(0);
}

int APS5_VABI _ZNK3sce2Np9CppWebApi6Common11TransactionINS2_12IntrusivePtrINS1_11UserProfile2V124GetBlockingUsersResponseEEENS4_INS2_18ResponseHeaderBaseEEEE11getResponseERS8_() {
    NetBridgeCall netCall("libSceNpCppWebApi", __func__);
    return netCall.Ret(SCE_NP_WEBAPI_ERROR_NOT_SIGNED_IN);
}

}

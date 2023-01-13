// Copyright 2019 Bloomberg Finance L.P
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

#include <digestgenerator.h>

#include <buildboxcommonmetrics_metricguard.h>
#include <buildboxcommonmetrics_totaldurationmetrictimer.h>
#include <env.h>

#include <iomanip>
#include <sstream>
#include <string>

#include <openssl/evp.h>

#define TIMER_NAME_CALCULATE_DIGESTS_TOTAL "recc.calculate_digests_total"

namespace recc {

namespace {

// If `status_code` is 0, throw an `std::runtime_error` exception with a
// description containing `function_name`. Otherwise, do
// nothing.
// (Note that this considers 0 an error, following the OpenSSL convention.)
void throwIfNotSuccessful(int status_code, const std::string &function_name)
{
    if (status_code == 0) {
        throw std::runtime_error(function_name + " failed.");
    }
    // "EVP_DigestInit_ex(), EVP_DigestUpdate() and EVP_DigestFinal_ex() return
    // 1 for success and 0 for failure."
    // https://openssl.org/docs/man1.1.0/man3/EVP_DigestInit.html
}

void deleteDigestContext(EVP_MD_CTX *context)
{
    EVP_MD_CTX_destroy(context);
    // ^ Calling this macro ensures compatibility with OpenSSL 1.0.2:
    // "EVP_MD_CTX_create() and EVP_MD_CTX_destroy() were
    // renamed to EVP_MD_CTX_new() and EVP_MD_CTX_free() in OpenSSL 1.1."
    // (https://openssl.org/docs/man1.1.0/man3/EVP_DigestInit.html)
}

// The context needs to be freed after use, so by storing it in an
// `unique_ptr` we ensure it is destroyed automatically even if we throw.
typedef std::unique_ptr<EVP_MD_CTX, decltype(&deleteDigestContext)>
    EVP_MD_CTX_ptr;

// Create and initialize an OpenSSL digest context to be used during a
// call to `hash()`.
EVP_MD_CTX_ptr createDigestContext(const EVP_MD *digestFunctionStruct)
{
    EVP_MD_CTX_ptr digest_context(EVP_MD_CTX_create(), &deleteDigestContext);
    // `EVP_MD_CTX_ptr` is an alias for `unique_ptr`, it will make sure that
    // the context object is freed if something goes wrong and we need to
    // throw.

    if (!digest_context) {
        throw std::runtime_error(
            "Error creating `EVP_MD_CTX` context struct.");
    }

    throwIfNotSuccessful(
        EVP_DigestInit_ex(digest_context.get(), digestFunctionStruct, nullptr),
        "EVP_DigestInit_ex()");

    return digest_context;
}

// Take a hash value produced by OpenSSL and return a string with its
// representation in hexadecimal.
std::string hashToHex(const unsigned char *hash_buffer, unsigned int hash_size)
{
    std::ostringstream ss;
    for (unsigned int i = 0; i < hash_size; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(hash_buffer[i]);
    }
    return ss.str();
}

// Get the OpenSSL MD struct for the digest function specified in the
// configuration. (Throws `out_of_range` for values not defined.)
const EVP_MD *getDigestFunctionStruct()
{
    // Translating the string set in the environment to a
    // `DigestFunction_Value`:
    const std::string digestFunctionName = RECC_CAS_DIGEST_FUNCTION;
    const proto::DigestFunction_Value digestValue =
        DigestGenerator::stringToDigestFunctionMap().at(digestFunctionName);

    // And from that value getting the OpenSSL MD corresponding to
    // that digest function:
    static const std::map<proto::DigestFunction_Value, const EVP_MD *>
        digestValueToOpenSslStructMap = {
            {proto::DigestFunction_Value_MD5, EVP_md5()},
            {proto::DigestFunction_Value_SHA1, EVP_sha1()},
            {proto::DigestFunction_Value_SHA256, EVP_sha256()},
            {proto::DigestFunction_Value_SHA384, EVP_sha384()},
            {proto::DigestFunction_Value_SHA512, EVP_sha512()}};

    return digestValueToOpenSslStructMap.at(digestValue);
}
} // namespace

proto::Digest DigestGenerator::make_digest(const std::string &blob)
{

    const EVP_MD *hashAlgorithm;
    try {
        hashAlgorithm = getDigestFunctionStruct();
    }
    catch (const std::out_of_range &) {
        throw std::runtime_error("Invalid or not supported digest function: " +
                                 RECC_CAS_DIGEST_FUNCTION);
    }

    std::string hash;
    { // Timed block
        buildboxcommon::buildboxcommonmetrics::MetricGuard<
            buildboxcommon::buildboxcommonmetrics::TotalDurationMetricTimer>
            mt(TIMER_NAME_CALCULATE_DIGESTS_TOTAL);

        // Initialize context:
        EVP_MD_CTX_ptr hashContext = createDigestContext(hashAlgorithm);
        // (Automatically destroyed)

        // Calculate hash:
        throwIfNotSuccessful(
            EVP_DigestUpdate(hashContext.get(), &blob[0], blob.length()),
            "EVP_DigestUpdate()");

        unsigned char hashBuffer[EVP_MAX_MD_SIZE];
        unsigned int messageLength;
        throwIfNotSuccessful(
            EVP_DigestFinal_ex(hashContext.get(), hashBuffer, &messageLength),
            "EVP_DigestFinal_ex()");

        // Generate hash string:
        hash = hashToHex(hashBuffer, static_cast<unsigned int>(messageLength));
    }

    proto::Digest result;
    result.set_hash(hash);
    result.set_size_bytes(static_cast<google::protobuf::int64>(blob.size()));
    return result;
}

proto::Digest
DigestGenerator::make_digest(const google::protobuf::MessageLite &message)
{
    return make_digest(message.SerializeAsString());
}

const std::map<std::string, proto::DigestFunction_Value> &
DigestGenerator::stringToDigestFunctionMap()
{
    static const std::map<std::string, proto::DigestFunction_Value>
        stringToFunctionMap = {{"MD5", proto::DigestFunction_Value_MD5},
                               {"SHA1", proto::DigestFunction_Value_SHA1},
                               {"SHA256", proto::DigestFunction_Value_SHA256},
                               {"SHA384", proto::DigestFunction_Value_SHA384},
                               {"SHA512", proto::DigestFunction_Value_SHA512}};

    return stringToFunctionMap;
}

std::string DigestGenerator::supportedDigestFunctionsList()
{

    const auto &map = DigestGenerator::stringToDigestFunctionMap();
    std::string res;

    auto it = map.cbegin();
    while (it != map.cend()) {
        const std::string digestName = it->first;
        res += ("\"" + digestName + "\"");

        it++;

        if (it != map.cend()) {
            res += ", ";
        }
    }

    return res;
}

} // namespace recc

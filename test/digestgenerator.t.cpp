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

#include <buildboxcommonmetrics_testingutils.h>
#include <buildboxcommonmetrics_totaldurationmetricvalue.h>
#include <digestgenerator.h>
#include <env.h>

#include <string>

#include <gtest/gtest.h>

#define TIMER_NAME_CALCULATE_DIGESTS_TOTAL "recc.calculate_digests_total"

using namespace recc;
using namespace buildboxcommon::buildboxcommonmetrics;
using proto::Digest;

TEST(CASHashTest, EmptyStringDefaultFunction)
{
    const auto digest = DigestGenerator::make_digest("");

    EXPECT_EQ(
        digest.hash(),
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

    EXPECT_EQ(digest.size_bytes(), 0);
}

TEST(CASHashTest, VerifyMetricsCollected)
{
    DigestGenerator::make_digest("");
    EXPECT_TRUE(collectedByName<TotalDurationMetricValue>(
        TIMER_NAME_CALCULATE_DIGESTS_TOTAL));
}

TEST(DigestGeneratorTest, StringDefaultFunction)
{
    using namespace std::literals::string_literals;
    const std::string testString(
        "This is a sample blob to hash. \0 It contains some NUL characters "
        "\0."s);

    const auto digest = DigestGenerator::make_digest(testString);

    const std::string expected_sha256_hash =
        "b1c4daf6e3812505064c07f1ad0b1d6693d93b1b28c452e55ad17e38c30e89aa";

    EXPECT_EQ(digest.hash(), expected_sha256_hash);
    EXPECT_EQ(digest.size_bytes(), testString.size());
}

TEST(DigestGeneratorTest, ProtoDefaultFunction)
{
    // Creating an arbitrary proto:
    proto::Command commandProto;
    *commandProto.add_arguments() = "command";
    *commandProto.add_arguments() = "-v";
    *commandProto.add_arguments() = "-1";
    *commandProto.add_output_files() = "output.txt";

    // Creating a digest from it:
    const Digest protoDigest = DigestGenerator::make_digest(commandProto);

    // And creating a digest from its serialization:
    const std::string serializedProto = commandProto.SerializeAsString();
    const Digest stringDigest = DigestGenerator::make_digest(serializedProto);

    EXPECT_EQ(protoDigest, stringDigest);
}

using namespace std::literals::string_literals;
const std::string TEST_STRING =
    "This is a sample blob to hash. \0 It contains some NUL characters "
    "\0."s;

TEST(DigestGeneratorTest, TestStringMD5)
{

    RECC_CAS_DIGEST_FUNCTION = "MD5";
    const Digest d = DigestGenerator::make_digest(TEST_STRING);

    const std::string expected_md5_string = "c1ad80398f865c700449c073bd0a8358";

    EXPECT_EQ(d.hash(), expected_md5_string);
    EXPECT_EQ(d.size_bytes(), TEST_STRING.size());
}

TEST(DigestGeneratorTest, TestStringSha1)
{
    RECC_CAS_DIGEST_FUNCTION = "SHA1";
    const Digest d = DigestGenerator::make_digest(TEST_STRING);

    const std::string expected_sha1_hash =
        "716e65700ad0e969cca29ec2259fa526e4bdb129";

    EXPECT_EQ(d.hash(), expected_sha1_hash);
    EXPECT_EQ(d.size_bytes(), TEST_STRING.size());
}

TEST(DigestGeneratorTest, TestStringSha256)
{
    RECC_CAS_DIGEST_FUNCTION = "SHA256";
    const Digest d = DigestGenerator::make_digest(TEST_STRING);

    const std::string expected_sha256_hash =
        "b1c4daf6e3812505064c07f1ad0b1d6693d93b1b28c452e55ad17e38c30e89aa";

    EXPECT_EQ(d.hash(), expected_sha256_hash);
    EXPECT_EQ(d.size_bytes(), TEST_STRING.size());
}

TEST(DigestGeneratorTest, TestStringSha384)
{
    RECC_CAS_DIGEST_FUNCTION = "SHA384";
    const Digest d = DigestGenerator::make_digest(TEST_STRING);

    const std::string expected_sha384_hash =
        "614589fe6e8bfd0e5a78e6819e439965364ec3af3a7482b69dd62e4ba47d82b5e305c"
        "b609d529164c794ba2b98e0279b";

    EXPECT_EQ(d.hash(), expected_sha384_hash);
    EXPECT_EQ(d.size_bytes(), TEST_STRING.size());
}

TEST(DigestGeneratorTest, TestStringSha512)
{
    RECC_CAS_DIGEST_FUNCTION = "SHA512";
    const Digest d = DigestGenerator::make_digest(TEST_STRING);

    const std::string expected_sha512_hash =
        "0e2c5c04c391ca0b8ca5fd9f6707bcddd53e8b7245c59331590d1c5490ffab7d505db"
        "0ba9b70a0f48e0f26ab6afeb84f600a7501a5fb1958f82f8623a7a1f692";

    EXPECT_EQ(d.hash(), expected_sha512_hash);
    EXPECT_EQ(d.size_bytes(), TEST_STRING.size());
}

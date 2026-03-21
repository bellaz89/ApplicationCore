// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#define BOOST_TEST_MODULE testLua

#include "Application.h"
#include "TestFacility.h"

#include <boost/test/included/unit_test.hpp>

using namespace boost::unit_test_framework;
namespace ctk = ChimeraTK;

namespace Tests::testLua {

  /********************************************************************************************************************/

  struct TestApp : public ctk::Application {
    explicit TestApp(const std::string& name) : ctk::Application(name) {}
    ~TestApp() override { shutdown(); }
  };

  /********************************************************************************************************************/
  /* Basic scalar push-pull module */

  BOOST_AUTO_TEST_CASE(testLuaModule) {
    std::cout << "***************************************************************************************" << std::endl;
    std::cout << "==> testLuaModule" << std::endl;

    TestApp app("testLuaSimpleApp");
    ctk::TestFacility tf(app);

    auto var1 = tf.getScalar<float>("/Var1");
    auto var2 = tf.getScalar<int32_t>("/Var2");

    tf.runApplication();

    // Check initial write (output = 0.5 before any input read)
    BOOST_TEST(var1.readNonBlocking());
    BOOST_TEST(float(var1) == 0.5f, boost::test_tools::tolerance(0.001f));

    // Write to input and step
    var2.setAndWrite(42);
    tf.stepApplication();
    BOOST_TEST(var1.readNonBlocking());
    BOOST_TEST(float(var1) == 42.5f, boost::test_tools::tolerance(0.001f));
  }

  /********************************************************************************************************************/
  /* Test initial values */

  BOOST_AUTO_TEST_CASE(testInitialValues) {
    std::cout << "***************************************************************************************" << std::endl;
    std::cout << "==> testInitialValues" << std::endl;

    TestApp app("testLuaSimpleApp");
    ctk::TestFacility tf(app);

    auto var1 = tf.getScalar<float>("/Var1");

    tf.setScalarDefault<int32_t>("/Var2", 10);
    tf.runApplication();

    // Module writes 0.5 (before first readAndGet); default for Var2 doesn't change this.
    BOOST_TEST(float(var1) == 0.5f, boost::test_tools::tolerance(0.001f));
  }

  /********************************************************************************************************************/
  /* Test array view semantics */

  BOOST_AUTO_TEST_CASE(testArrays) {
    std::cout << "***************************************************************************************" << std::endl;
    std::cout << "==> testArrays" << std::endl;

    TestApp app("testLuaWithArray");
    ctk::TestFacility tf(app);

    auto arrayIn1  = tf.getArray<int32_t>("/SomeName/ArrayIn1");
    auto arrayIn2  = tf.getArray<int32_t>("/SomeName/ArrayIn2");
    auto arrayOut1 = tf.getArray<int32_t>("/SomeName/ArrayOut1");
    auto arrayOut2 = tf.getArray<int32_t>("/SomeName/ArrayOut2");
    auto testError = tf.getScalar<std::string>("/SomeName/TestError");

    // Set initial value for ArrayIn1
    tf.setArrayDefault<int32_t>("/SomeName/ArrayIn1", {50, 5});

    tf.runApplication();

    // Check initial output: sum(50,5) + offset = 55, 56, 57, ...
    std::vector<int32_t> ref(10);
    for(int i = 0; i < 10; ++i) ref[i] = 55 + i;
    BOOST_TEST(std::vector<int32_t>(arrayOut1) == ref, boost::test_tools::per_element());

    // Write ArrayIn2, step → check ArrayOut2 = sum + offset
    arrayIn2 = {2, 3, 4, 5, 6};
    arrayIn2.write();
    tf.stepApplication();
    BOOST_TEST(arrayOut2.readNonBlocking());
    for(int i = 0; i < 10; ++i) ref[i] = 20 + i; // sum = 2+3+4+5+6 = 20
    BOOST_TEST(std::vector<int32_t>(arrayOut2) == ref, boost::test_tools::per_element());

    // Write ArrayIn1 again → check ArrayOut1
    arrayIn1 = {100, 20};
    arrayIn1.write();
    tf.stepApplication();
    BOOST_TEST(arrayOut1.readNonBlocking());
    for(int i = 0; i < 10; ++i) ref[i] = 120 + i;
    BOOST_TEST(std::vector<int32_t>(arrayOut1) == ref, boost::test_tools::per_element());

    // Check no Lua-side errors
    BOOST_TEST(testError.readNonBlocking() == false);
    BOOST_TEST(std::string(testError) == "");
  }

  /********************************************************************************************************************/
  /* Test ConfigReader (appConfig) */

  BOOST_AUTO_TEST_CASE(testAppConfig) {
    std::cout << "***************************************************************************************" << std::endl;
    std::cout << "==> testAppConfig" << std::endl;

    TestApp app("testLuaAppConfig");
    ctk::TestFacility tf(app);

    auto result = tf.getScalar<std::string>("/UserModule/testError");

    tf.runApplication();
    result.readLatest();
    BOOST_TEST(std::string(result) == "");
  }

  /********************************************************************************************************************/

} // namespace Tests::testLua

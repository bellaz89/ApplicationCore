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
    var1.readLatest();
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
  /* Test Lua appConfig() bindings */

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
  /* Test method-call accessor factories, upvalue migration, appConfig(), log() */

  BOOST_AUTO_TEST_CASE(testNewAPI) {
    std::cout << "***************************************************************************************" << std::endl;
    std::cout << "==> testNewAPI" << std::endl;

    TestApp app("testLuaNewAPI");
    ctk::TestFacility tf(app);

    auto input  = tf.getScalar<float>("/NewAPI/input");
    auto output = tf.getScalar<float>("/NewAPI/output");
    auto status = tf.getScalar<std::string>("/NewAPI/status");

    tf.runApplication();

    // Check initial writes (before any input read)
    output.readLatest();
    BOOST_TEST(float(output) == 0.0f, boost::test_tools::tolerance(0.001f));
    status.readLatest();
    BOOST_TEST(std::string(status) == "ready");

    // Write input, step — verify output = input * scale (scale=2.0)
    input.setAndWrite(3.0f);
    tf.stepApplication();
    BOOST_TEST(output.readNonBlocking());
    BOOST_TEST(float(output) == 6.0f, boost::test_tools::tolerance(0.001f));
    BOOST_TEST(status.readNonBlocking());
    BOOST_TEST(std::string(status) == "ok");
  }

  /********************************************************************************************************************/
  /* Test upvalue migration: file-scope locals available in mainLoop */

  BOOST_AUTO_TEST_CASE(testUpvalueMigration) {
    std::cout << "***************************************************************************************" << std::endl;
    std::cout << "==> testUpvalueMigration" << std::endl;

    TestApp app("testLuaUpvalue");
    ctk::TestFacility tf(app);

    auto input  = tf.getScalar<float>("/Upvalue/input");
    auto output = tf.getScalar<float>("/Upvalue/output");
    auto label  = tf.getScalar<std::string>("/Upvalue/label");

    tf.runApplication();

    // Check initial writes
    output.readLatest();
    BOOST_TEST(float(output) == 0.0f, boost::test_tools::tolerance(0.001f));
    label.readLatest();
    BOOST_TEST(std::string(label) == "v=init");

    // Write input = 5.0, step — verify output = 5.0 + 10.0 = 15.0, label = "v=15"
    input.setAndWrite(5.0f);
    tf.stepApplication();
    BOOST_TEST(output.readNonBlocking());
    BOOST_TEST(float(output) == 15.0f, boost::test_tools::tolerance(0.001f));
    BOOST_TEST(label.readNonBlocking());
    BOOST_TEST(std::string(label) == "v=15");
  }

  /********************************************************************************************************************/
  /* Test arithmetic metamethods on scalar accessors */

  BOOST_AUTO_TEST_CASE(testArithmeticMetamethods) {
    std::cout << "***************************************************************************************" << std::endl;
    std::cout << "==> testArithmeticMetamethods" << std::endl;

    TestApp app("testLuaArithmetic");
    ctk::TestFacility tf(app);

    auto a    = tf.getScalar<float>("/Arith/a");
    auto b    = tf.getScalar<float>("/Arith/b");
    auto sum  = tf.getScalar<float>("/Arith/sum");
    auto diff = tf.getScalar<float>("/Arith/diff");
    auto prod = tf.getScalar<float>("/Arith/prod");
    auto quot = tf.getScalar<float>("/Arith/quot");
    auto neg  = tf.getScalar<float>("/Arith/neg");
    auto lt   = tf.getScalar<ChimeraTK::Boolean>("/Arith/lt");

    tf.runApplication();
    a.setAndWrite(3.0f);
    b.setAndWrite(4.0f);
    tf.stepApplication();

    // Check arithmetic computation
    sum.readLatest();
    BOOST_TEST(float(sum)  == 7.0f,  boost::test_tools::tolerance(0.001f));
    diff.readLatest();
    BOOST_TEST(float(diff) == -1.0f, boost::test_tools::tolerance(0.001f));
    prod.readLatest();
    BOOST_TEST(float(prod) == 12.0f, boost::test_tools::tolerance(0.001f));
    quot.readLatest();
    BOOST_TEST(float(quot) == 0.75f, boost::test_tools::tolerance(0.001f));
    neg.readLatest();
    BOOST_TEST(float(neg)  == -3.0f, boost::test_tools::tolerance(0.001f));
    lt.readLatest();
    BOOST_TEST(static_cast<bool>(lt) == true);
  }

  /********************************************************************************************************************/
  /* Test ReadAnyGroup binding (port of testPythonReadAnyGroup) */

  BOOST_AUTO_TEST_CASE(testReadAnyGroup) {
    std::cout << "***************************************************************************************" << std::endl;
    std::cout << "==> testReadAnyGroup" << std::endl;

    TestApp app("testLuaReadAnyGroup");
    ctk::TestFacility tf(app);

    auto in1       = tf.getScalar<int32_t>("/UserModule/in1");
    auto in2       = tf.getArray <int32_t>("/UserModule/in2");
    auto in3       = tf.getScalar<int32_t>("/UserModule/in3");
    auto out       = tf.getScalar<std::string>("/UserModule/output");
    auto testError = tf.getScalar<std::string>("/UserModule/testError");

    tf.runApplication();

    // step1: in1 update is consumed by readAny
    in1.setAndWrite(12);
    tf.stepApplication();
    BOOST_CHECK(out.readNonBlocking());
    BOOST_TEST(std::string(out) == "step1");

    // step2: in2 update is consumed by readAny
    in2 = {24, 24, 24, 24};
    in2.write();
    tf.stepApplication();
    BOOST_CHECK(out.readNonBlocking());
    BOOST_TEST(std::string(out) == "step2");

    // step3: in3 is read directly (not part of the group); group should report no pending updates
    in3.setAndWrite(36);
    tf.stepApplication();
    BOOST_CHECK(out.readNonBlocking());
    BOOST_TEST(std::string(out) == "step3");

    // step4: readUntil(accessor) form
    in1.setAndWrite(8);
    tf.stepApplication();
    BOOST_CHECK(out.readNonBlocking());
    BOOST_TEST(std::string(out) == "step4");

    // step5: readUntil(accessor) on the array input
    in2 = {16, 16, 16, 16};
    in2.write();
    tf.stepApplication();
    BOOST_CHECK(out.readNonBlocking());
    BOOST_TEST(std::string(out) == "step5");

    // step6: readUntil(id) form
    in1.setAndWrite(42);
    tf.stepApplication();
    BOOST_CHECK(out.readNonBlocking());
    BOOST_TEST(std::string(out) == "step6");

    BOOST_CHECK(!testError.readNonBlocking());
    BOOST_TEST(std::string(testError) == "");
  }

  /********************************************************************************************************************/
  /* Test DataConsistencyGroup binding (port of testPythonDataConsistencyGroup, simplified) */

  BOOST_AUTO_TEST_CASE(testDataConsistencyGroup) {
    std::cout << "***************************************************************************************" << std::endl;
    std::cout << "==> testDataConsistencyGroup" << std::endl;

    TestApp app("testLuaDataConsistencyGroup");
    ctk::TestFacility tf(app);

    auto testError = tf.getScalar<std::string>("/UserModule/testError");

    tf.runApplication();

    // The sender + receiver run their full sequence inside runApplication's testable-mode
    // settle phase: prepare() seeds initial values, then both mainLoops drain the queued
    // updates and the receiver writes its verdict. Mirrors testPythonDataConsistencyGroup.
    testError.readLatest();
    BOOST_TEST(std::string(testError) == "ok");
  }

  /********************************************************************************************************************/

} // namespace Tests::testLua

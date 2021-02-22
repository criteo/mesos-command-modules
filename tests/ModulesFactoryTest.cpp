#include "ModulesFactory.hpp"
#include <gtest/gtest.h>
#include "CommandHook.hpp"
#include "CommandIsolator.hpp"
#include "CommandResourceEstimator.hpp"
#include "CommandQoSController.hpp"

using namespace criteo::mesos;

// ***************************************
// **************** Hook *****************
// ***************************************
TEST(ModulesFactoryTest, should_create_hook_with_correct_parameters) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("module_name");
  var->set_value("test");

  var = parameters.add_parameter();
  var->set_key("hook_slave_run_task_label_decorator_command");
  var->set_value("command_slave_run_task_label_decorator");

  var = parameters.add_parameter();
  var->set_key("hook_slave_executor_environment_decorator_command");
  var->set_value("command_slave_executor_environment_decorator");

  var = parameters.add_parameter();
  var->set_key("hook_slave_remove_executor_hook_command");
  var->set_value("command_slave_remove_executor_hook");

  std::unique_ptr<CommandHook> hook(
      dynamic_cast<CommandHook*>(createHook(parameters)));

  ASSERT_EQ(hook->runTaskLabelCommand().get(),
            Command("command_slave_run_task_label_decorator", 30));
  ASSERT_EQ(hook->executorEnvironmentCommand().get(),
            Command("command_slave_executor_environment_decorator", 30));
  ASSERT_EQ(hook->removeExecutorCommand().get(),
            Command("command_slave_remove_executor_hook", 30));
}

TEST(ModulesFactoryTest, should_create_hook_with_empty_parameters) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("module_name");
  var->set_value("test");

  var = parameters.add_parameter();
  var->set_key("hook_slave_executor_environment_decorator_command");
  var->set_value("command_slave_executor_environment_decorator");

  std::unique_ptr<CommandHook> hook(
      dynamic_cast<CommandHook*>(createHook(parameters)));

  ASSERT_TRUE(hook->runTaskLabelCommand().isNone());
  ASSERT_EQ(hook->executorEnvironmentCommand().get(),
            Command("command_slave_executor_environment_decorator", 30));
  ASSERT_TRUE(hook->removeExecutorCommand().isNone());
}

// ***************************************
// ************** Isolator ***************
// ***************************************
TEST(ModulesFactoryTest, should_create_isolator_with_correct_parameters) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("module_name");
  var->set_value("test");

  var = parameters.add_parameter();
  var->set_key("isolator_prepare_command");
  var->set_value("command_prepare");

  var = parameters.add_parameter();
  var->set_key("isolator_isolate_command");
  var->set_value("command_isolate");

  var = parameters.add_parameter();
  var->set_key("isolator_cleanup_command");
  var->set_value("command_cleanup");



  std::unique_ptr<CommandIsolator> isolator(
      dynamic_cast<CommandIsolator*>(createIsolator(parameters)));

  ASSERT_EQ(isolator->prepareCommand().get(), Command("command_prepare", 30));
  ASSERT_EQ(isolator->isolateCommand().get(), Command("command_isolate", 30));
  ASSERT_EQ(isolator->cleanupCommand().get(), Command("command_cleanup", 30));
}

TEST(ModulesFactoryTest, should_create_isolator_with_empty_parameters) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("module_name");
  var->set_value("test");

  var = parameters.add_parameter();
  var->set_key("isolator_prepare_command");
  var->set_value("command_prepare");

  std::unique_ptr<CommandIsolator> isolator(
      dynamic_cast<CommandIsolator*>(createIsolator(parameters)));

  ASSERT_EQ(isolator->prepareCommand().get(), Command("command_prepare", 30));
  ASSERT_TRUE(isolator->cleanupCommand().isNone());
  ASSERT_TRUE(isolator->isolateCommand().isNone());
}

// ***************************************
// ********* ResourceEstimator ***********
// ***************************************

//mock resourceUsage
static const process::Future<::mesos::ResourceUsage> MockUsage(){
     
  process::Owned<mesos::ResourceUsage> usage(new ::mesos::ResourceUsage());

  mesos::Resources mockResources = mesos::Resources::parse("cpus:1; mem:128").get();
  //mock Executorinfo
  mesos::ExecutorInfo executorInfo;
  executorInfo.set_name("test_exec");
  mockResources.allocate("*");
  executorInfo.mutable_resources()->CopyFrom(mockResources);
  executorInfo.mutable_executor_id()->set_value("mock.executor.Id");
  {
    auto env = executorInfo.mutable_command()->mutable_environment();
    auto var = env->add_variables();
    var->set_name("foo");
    var->set_value("bar");
    var = env->add_variables();
    var->set_name("deleted");
    var->set_value("whatever");
  }

  //mock TaskInfo
  mesos::TaskInfo taskInfo;
  taskInfo.set_name("test_task");
  taskInfo.mutable_task_id()->set_value("1");
  taskInfo.mutable_slave_id()->set_value("2");
  {
    auto l = taskInfo.mutable_labels()->add_labels();
    l->set_key("foo");
    l->set_value("bar");
  }
  
  mesos::Resources allocatedResources = mesos::Resources::parse(
        "cpus:" + stringify("1") +
        ";mem:" + stringify("32")).get();
  allocatedResources.allocate("*");

  //mockExecutor
  mesos::ResourceUsage::Executor* mockExecutor = usage->add_executors();
  mockExecutor->mutable_executor_info()->CopyFrom(executorInfo);
  mockExecutor->mutable_allocated()->CopyFrom(allocatedResources);
  mockExecutor->mutable_container_id()->set_value("3");

  //mocktask
  /*mesos::ResourceUsage::Executor::Task mockTask = mockExecutor->add_tasks();
  mockTask.set_name("test");
  mockTask.mutable_id()->set_value("1");
  mockTask.mutable_resources()->CopyFrom(mockResources);*/

  //mock total 
  mesos::Resources totalResources = mesos::Resources::parse(
	"cpus: 4 ;mem : 1024").get();
  usage->mutable_total()->CopyFrom(totalResources);
  return process::Future<mesos::ResourceUsage>(*usage);
};

TEST(ModulesFactoryTest, should_create_resourceEstimator_with_correct_parameters) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("module_name");
  var->set_value("test");  
  var = parameters.add_parameter();
  var->set_key("resource_estimator_oversubscribable_command");
  var->set_value("command_oversubscribable");

  std::unique_ptr<CommandResourceEstimator> resourceEstimator(
      dynamic_cast<CommandResourceEstimator*>(createResourceEstimator(parameters)));
  resourceEstimator->initialize(MockUsage);
  ASSERT_EQ(resourceEstimator->oversubscribableCommand().get(), Command("command_oversubscribable", 30));
}

TEST(ModulesFactoryTest, should_create_resourceEstimator_with_empty_parameters) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("module_name");
  var->set_value("test");

  std::unique_ptr<CommandResourceEstimator> resourceEstimator(
      dynamic_cast<CommandResourceEstimator*>(createResourceEstimator(parameters)));
  resourceEstimator->initialize(MockUsage);
  ASSERT_TRUE(resourceEstimator->oversubscribableCommand().isNone());
}


// ***************************************
// *********** QoSController *************
// ***************************************

TEST(ModulesFactoryTest, should_create_QoSController_with_correct_parameters) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("module_name");
  var->set_value("test");  
  var = parameters.add_parameter();
  var->set_key("qoscontroller_corrections_command");
  var->set_value("command_corrections");

  std::unique_ptr<CommandQoSController> QoSController(
      dynamic_cast<CommandQoSController*>(createQoSController(parameters)));
  QoSController->initialize(MockUsage);
  ASSERT_EQ(QoSController->correctionsCommand().get(), Command("command_corrections", 30));
}

TEST(ModulesFactoryTest, should_create_QoSController_with_empty_parameters) {
  ::mesos::Parameters parameters;
  auto var = parameters.add_parameter();
  var->set_key("module_name");
  var->set_value("test");

  std::unique_ptr<CommandQoSController> QoSController(
      dynamic_cast<CommandQoSController*>(createQoSController(parameters)));
  QoSController->initialize(MockUsage);
  ASSERT_TRUE(QoSController->correctionsCommand().isNone());
}


#include "CommandResourceEstimator.hpp"
#include "gtest_helpers.hpp"

#include <gtest/gtest.h>
#include <process/gtest.hpp>
#include <stout/lambda.hpp>
#include <mesos/resources.hpp>

#include <google/protobuf/util/json_util.h>

extern std::string g_resourcesPath;

using namespace criteo::mesos;
using process::Owned;

const int32_t CPU_ALLOCATED = 1;
const int32_t MEM_ALLOCATED = 128;

class CommandResourceEstimatorTest : public ::testing::Test {
  public:
	//mock resourceUsage
	static const process::Future<::mesos::ResourceUsage> MockUsage(){
    
	Owned<mesos::ResourceUsage> usage(new ::mesos::ResourceUsage());
   
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
          "cpus:" + stringify(CPU_ALLOCATED) +
          ";mem:" + stringify(MEM_ALLOCATED)).get();
    allocatedResources.allocate("*");
  
    //mockExecutor
    mesos::ResourceUsage::Executor* mockExecutor = usage->add_executors();
    mockExecutor->mutable_executor_info()->CopyFrom(executorInfo);
    mockExecutor->mutable_allocated()->CopyFrom(allocatedResources);
    mockExecutor->mutable_container_id()->set_value("3");

	//mock total 
    mesos::Resources totalResources = mesos::Resources::parse(
	    "cpus: 4 ;mem : 1024").get();
    usage->mutable_total()->CopyFrom(totalResources);
    return process::Future<mesos::ResourceUsage>(*usage);
    }

    virtual void SetUp(); 
     
  	const lambda::function<process::Future<mesos::ResourceUsage>()> resourceUsage;  
  	std::unique_ptr<CommandResourceEstimator> resourceEstimator;
};

   void CommandResourceEstimatorTest::SetUp(){
	    //resourceUsage = &MockUsage;
	resourceEstimator.reset(new CommandResourceEstimator("test",
 	    	Command(g_resourcesPath + "oversubscribable.sh"), false));
      resourceEstimator->initialize(MockUsage);
   };

 class CommandResourceEstimatorSimpleTest : public CommandResourceEstimatorTest {
    
  public:
    void SetUp() {
	  CommandResourceEstimatorTest::SetUp();
	}
};

  TEST_F(CommandResourceEstimatorSimpleTest, should_run_oversubscribable_command) {
    
	SetUp();

    mesos::Resources resources = mesos::Resources::parse("[{\"name\" : \"cpus\", \"type\" : \"SCALAR\", \"scalar\" : {\"value\" : \"8\"}, \"role\" : \"*\"}]").get();
    mesos::Resources revocable{};
    foreach (mesos::Resource resource, resources){
      resource.mutable_revocable();
      revocable+=resource;
    }
    mesos::Resources output;
	if (resourceEstimator)
    	output = resourceEstimator->oversubscribable().get();
    else
        output = mesos::Resources();
    EXPECT_EQ(output, revocable); 
}


  class CommandResourceEstimatorEmptyTest : public CommandResourceEstimatorTest {
    
  public:
    void SetUp() {
	  CommandResourceEstimatorTest::SetUp();
	  resourceEstimator.reset(new CommandResourceEstimator("test",
	    Command(g_resourcesPath + "oversubscribable_empty.sh"), false));
	  resourceEstimator->initialize(MockUsage);
    }
};


  TEST_F(CommandResourceEstimatorEmptyTest, should_not_run_oversubscribable_command) {
    
	SetUp();

    mesos::Resources resources = mesos::Resources::parse("[{\"name\" : \"cpus\", \"type\" : \"SCALAR\", \"scalar\" : {\"value\" : \"0\"}, \"role\" : \"*\"}]").get();
    mesos::Resources revocable{};
    foreach (mesos::Resource resource, resources){
      resource.mutable_revocable();
      revocable+=resource;
    }
    mesos::Resources output;
	if (resourceEstimator)
    	output = resourceEstimator->oversubscribable().get();
    else
        output = mesos::Resources();
    EXPECT_EQ(output, resources); 
}


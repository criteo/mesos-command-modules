#include "CommandQoSController.hpp"
#include "gtest_helpers.hpp"

#include <gtest/gtest.h>
#include <process/gtest.hpp>
#include <stout/lambda.hpp>
#include <mesos/resources.hpp>

#include <google/protobuf/util/json_util.h>

extern std::string g_resourcesPath;

using namespace criteo::mesos;
using process::Owned;
using std::list;

const int32_t CPU_ALLOCATED = 1;
const int32_t MEM_ALLOCATED = 128;

class CommandQoSControllerTest : public ::testing::Test {
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
  	std::unique_ptr<CommandQoSController> qosController;
};

   void CommandQoSControllerTest::SetUp(){
	    //resourceUsage = &MockUsage;
	qosController.reset(new CommandQoSController("test",
 	    	Command(g_resourcesPath + "corrections.sh"), false));
      qosController->initialize(MockUsage);
   };

 class CommandQoSControllerSimpleTest : public CommandQoSControllerTest {
    
  public:
    void SetUp() {
	  CommandQoSControllerTest::SetUp();
    }
};

  TEST_F(CommandQoSControllerSimpleTest, should_run_corrections_command) {
    
	SetUp();

	::mesos::slave::QoSCorrection correction;
	::mesos::slave::QoSCorrection::Kill *kill = correction.mutable_kill();
	kill->mutable_framework_id()->set_value("1");
	kill->mutable_executor_id()->set_value("revocable");
	kill->mutable_container_id()->set_value("0");
	//string strCorrection = "{\"type\":\"KILL\",\"kill\":{\"executorId\":{\"value\":\"revocable\"},\"frameworkId\":{\"value\":\"1\"},\"containerId\":{\"value\":\"2\"}}}";
	//google::protobuf::util::JsonStringToMessage(strCorrection, &correction);
    list<::mesos::slave::QoSCorrection> corrections({correction});
	list<::mesos::slave::QoSCorrection> output;

	output = qosController->corrections().get();
	auto it1 = output.begin();
	auto it2 = corrections.begin();
	while(it1 != output.end() && it2 != corrections.end()) { 
      EXPECT_EQ(it1, it2);
	  it1++;
	  it2++;
	} 
}


  class CommandQoSControllerEmptyTest : public CommandQoSControllerTest {
    
  public:
    void SetUp() {
	  CommandQoSControllerTest::SetUp();
	  qosController.reset(new CommandQoSController("test",
	    Command(g_resourcesPath + "corrections_empty.sh"), false));
	  qosController->initialize(MockUsage);
    }
};


  TEST_F(CommandQoSControllerEmptyTest, should_not_run_corrections_command) {
    
	SetUp();
	list<::mesos::slave::QoSCorrection> output = qosController->corrections().get();
	list<::mesos::slave::QoSCorrection> corrections = list<::mesos::slave::QoSCorrection>();	
    
	auto it1 = output.begin();
	auto it2 = corrections.begin();
	while(it1 != output.end() && it2 != corrections.end()) { 
      EXPECT_EQ(it1, it2);
	  it1++;
	  it2++;
	}  
}

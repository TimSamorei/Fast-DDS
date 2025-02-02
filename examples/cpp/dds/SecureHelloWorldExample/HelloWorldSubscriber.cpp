// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

/**
 * @file HelloWorldSubscriber.cpp
 *
 */

#include "HelloWorldSubscriber.h"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>

using namespace eprosima::fastdds::dds;

HelloWorldSubscriber::HelloWorldSubscriber()
    : participant_(nullptr)
    , subscriber_(nullptr)
    , topic_(nullptr)
    , reader_(nullptr)
    , type_(new HelloWorldPubSubType())
{
}

bool HelloWorldSubscriber::init()
{
    //CREATE THE PARTICIPANT
    DomainParticipantQos pqos;

    pqos.properties().properties().emplace_back("dds.sec.auth.plugin",
            "builtin.PKI-DH");
    pqos.properties().properties().emplace_back("dds.sec.auth.builtin.PKI-DH.identity_ca",
            "file://certs/maincacert.pem");
    pqos.properties().properties().emplace_back("dds.sec.auth.builtin.PKI-DH.identity_certificate",
            "file://certs/mainsubcert.pem");
    pqos.properties().properties().emplace_back("dds.sec.auth.builtin.PKI-DH.private_key",
            "file://certs/mainsubkey.pem");
    pqos.properties().properties().emplace_back("dds.sec.access.plugin",
            "builtin.Access-Permissions");
    pqos.properties().properties().emplace_back(
        "dds.sec.access.builtin.Access-Permissions.permissions_ca",
        "file://certs/maincacert.pem");
    pqos.properties().properties().emplace_back(
        "dds.sec.access.builtin.Access-Permissions.governance",
        "file://certs/governance.smime");
    pqos.properties().properties().emplace_back(
        "dds.sec.access.builtin.Access-Permissions.permissions",
        "file://certs/permissions.smime");
    pqos.properties().properties().emplace_back("dds.sec.crypto.plugin",
            "builtin.AES-GCM-GMAC");

    pqos.properties().properties().emplace_back("dds.sec.auth.plugin",
            "builtin.TC-RA");
    pqos.properties().properties().emplace_back("dds.sec.auth.builtin.TC-RA.sig_key",
            "HS/SRK/mySignKey");
    pqos.properties().properties().emplace_back("dds.sec.auth.builtin.TC-RA.verify_key",
            "/ext/myExtPubKey");
    pqos.properties().properties().emplace_back("dds.sec.auth.builtin.TC-RA.pcr_list",
            "16");
    pqos.properties().properties().emplace_back("dds.sec.auth.builtin.TC-RA.pcr_num",
            "1");

    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, pqos);

    if (participant_ == nullptr)
    {
        return false;
    }

    //REGISTER THE TYPE
    type_.register_type(participant_);

    //CREATE THE SUBSCRIBER
    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);

    if (subscriber_ == nullptr)
    {
        return false;
    }

    //CREATE THE TOPIC
    topic_ = participant_->create_topic("HelloWorldTopic", "HelloWorld", TOPIC_QOS_DEFAULT);
    //CREATE THE DATAREADER
    DataReaderQos rqos;
    rqos.history().kind = KEEP_LAST_HISTORY_QOS;
    rqos.history().depth = 30;
    rqos.resource_limits().max_samples = 50;
    rqos.resource_limits().allocated_samples = 20;
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;

    reader_ = subscriber_->create_datareader(topic_, rqos, &listener_);

    if (reader_ == nullptr)
    {
        return false;
    }

    return true;
}

HelloWorldSubscriber::~HelloWorldSubscriber()
{
    if (reader_ != nullptr)
    {
        subscriber_->delete_datareader(reader_);
    }
    if (topic_ != nullptr)
    {
        participant_->delete_topic(topic_);
    }
    if (subscriber_ != nullptr)
    {
        participant_->delete_subscriber(subscriber_);
    }
    DomainParticipantFactory::get_instance()->delete_participant(participant_);
}

void HelloWorldSubscriber::SubListener::on_subscription_matched(
        DataReader*,
        const SubscriptionMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        matched_ = info.total_count;
        std::cout << "Subscriber matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        matched_ = info.total_count;
        std::cout << "Subscriber unmatched." << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
                  << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
    }
}

void HelloWorldSubscriber::SubListener::on_data_available(
        DataReader* reader)
{
    SampleInfo info;
    if (reader->take_next_sample(&hello_, &info) == ReturnCode_t::RETCODE_OK)
    {
        if (info.instance_state == ALIVE_INSTANCE_STATE)
        {
            samples_++;
            // Print your structure data here.
            std::cout << "Message " << hello_.message() << " " << hello_.index() << " RECEIVED" << std::endl;
        }
    }
}

void HelloWorldSubscriber::run()
{
    std::cout << "Subscriber running. Please press enter to stop the Subscriber" << std::endl;
    std::cin.ignore();
}

void HelloWorldSubscriber::run(
        uint32_t number)
{
    std::cout << "Subscriber running until " << number << "samples have been received" << std::endl;
    while (number < this->listener_.samples_)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

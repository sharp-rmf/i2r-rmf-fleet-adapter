#pragma once

#include <websocketpp/config/asio_client.hpp>

#include <websocketpp/client.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <memory>

#include "i2r_driver/i2r_driver.hpp"
#include "i2r_driver/mission_gen.hpp"
#include "i2r_driver/feedback_parser.hpp"

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;

class connection_metadata {
public:
    typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;
    typedef std::shared_ptr<boost::asio::ssl::context> context_ptr;
    using FleetStatePub =
      rclcpp::Publisher<rmf_fleet_msgs::msg::FleetState>::SharedPtr;

    connection_metadata(
        int id, 
        websocketpp::connection_hdl hdl, 
        std::string uri,
        FleetStatePub fleet_state_pub)
      : m_id(id)
      , m_hdl(hdl)
      , m_uri(uri)
      , m_fleet_state_pub(fleet_state_pub)
      , m_status("Connecting")
      , m_server("N/A")
      
    {}

    static context_ptr on_tls_init(websocketpp::connection_hdl);

    void on_open(client * c, websocketpp::connection_hdl hdl);

    void on_fail(client * c, websocketpp::connection_hdl hdl);
    
    void on_close(client * c, websocketpp::connection_hdl hdl);

    void on_message(websocketpp::connection_hdl, client::message_ptr msg);

    websocketpp::connection_hdl get_hdl() const {
        return m_hdl;
    }
    
    int get_id() const {
        return m_id;
    }
    
    std::string get_status() const {
        return m_status;
    }

    void record_sent_message(std::string message) {
        m_messages.push_back(">> " + message);
    }
    std::vector<std::string> m_messages;

    rmf_fleet_msgs::msg::FleetState fs_msg;
    rmf_fleet_msgs::msg::PathRequest path_request_msg;
    std::unique_ptr<std::vector<double>>  map_coordinate_transformation_ptr;
    int path_compeletion_status =-1;
    std::string task_id = "empty";
private:
    std::mutex _mtx;
    FleetStatePub m_fleet_state_pub;
    int m_id;
    websocketpp::connection_hdl m_hdl;
    std::string m_uri;
    std::string m_status;
    std::string m_server;
    std::string m_error_reason;
};

class websocket_endpoint {
public:

    using FleetStatePub =
      rclcpp::Publisher<rmf_fleet_msgs::msg::FleetState>::SharedPtr;

    websocket_endpoint (
        FleetStatePub fleet_state_pub) : 
        _fleet_state_pub (std::move(fleet_state_pub)),
        m_next_id(0)
    {
        m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
        m_endpoint.clear_error_channels(websocketpp::log::elevel::all);

        m_endpoint.init_asio();
        m_endpoint.start_perpetual();

        m_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(&client::run, &m_endpoint);
    
        std::cout<<"Websocket_endpoint Connection started"<<std::endl;
  }

    ~websocket_endpoint() {
        m_endpoint.stop_perpetual();
        for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
            if (it->second->get_status() != "Open") {
                // Only close open connections
                continue;
            }
            
            std::cout << "> Closing connection " << it->second->get_id() << std::endl;
            
            websocketpp::lib::error_code ec;
            m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
            if (ec) {
                std::cout << "> Error closing connection " << it->second->get_id() << ": "  
                          << ec.message() << std::endl;
            }
        }
        std::cout<<"Websocket_endpoint Connection closed"<<std::endl;
        m_thread->join();
    }

    int connect(std::string const & uri);

    void close(int id, websocketpp::close::status::value code, std::string reason);
    void send(int id, std::string message, websocketpp::lib::error_code& e);
    void send(int id, std::string message);

    connection_metadata::ptr get_metadata(int id) const;

    typedef std::map<int,connection_metadata::ptr> con_list;
    
    
    con_list m_connection_list;

private:
    FleetStatePub _fleet_state_pub;
    client m_endpoint;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

    int m_next_id;
};
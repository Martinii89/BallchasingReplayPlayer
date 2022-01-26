#pragma once

#define ASIO_STANDALONE
 
#include "websocketpp/config/asio_no_tls.hpp"
#include "websocketpp/server.hpp"

#include <set>
#include <thread>


class WsServer
{
public:
	using Server = websocketpp::server<websocketpp::config::asio>;
	using MessagePtr = Server::message_ptr;
	using ConnectionPtr = Server::connection_ptr;
	using ClientList = std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>>;
	using MessageHandler = std::function<void(Server* server, websocketpp::connection_hdl const& hdl, MessagePtr const& message)>;
	using MessageHandlerKey = std::string;
	
	void StartWebSocketServerAsync(uint16_t port);
	void ShutdownWebSocketServer();
	void AddMessageHandler(const MessageHandlerKey& key, MessageHandler message_handler);
	void Send(websocketpp::connection_hdl const& hdl, std::string const& msg);

	~WsServer();
	WsServer() = default;
	
	// no copy and no move
	WsServer(const WsServer& other) = delete;
	WsServer(WsServer&& other) noexcept = delete;
	WsServer& operator=(const WsServer& other) = delete;
	WsServer& operator=(WsServer&& other) noexcept = delete;
private:
	Server ws_server_;
	ClientList ws_connections_;
	std::thread server_thread_;
	std::mutex server_running_mutex_;
	std::unordered_map<MessageHandlerKey, MessageHandler> message_handlers_;

	uint16_t port_ = -1;

	void InitAsio();
	void StartWebSocketServer(uint16_t port);
	
	void OnMessage(const websocketpp::connection_hdl& hdl, const MessagePtr& message);
	void OnClientConnected(websocketpp::connection_hdl hdl);
	void OnClientDisconnected(websocketpp::connection_hdl hdl);
};
